#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define PAGE_SIZE 4096
#define NUM_FRAMES 16
#define NUM_PROCESSES 5
#define PAGE_TABLE_SIZE 256
#define PHYSICAL_MEMORY_SIZE (NUM_FRAMES * PAGE_SIZE)

typedef struct {
    int frame_number;
    bool valid;
    int last_used;
} PageTableEntry;

typedef struct {
    PageTableEntry page_table[PAGE_TABLE_SIZE];
} Process;

typedef struct {
    int frames[NUM_FRAMES];
    int size;
} FreeFrameList;

Process process[NUM_PROCESSES];
FreeFrameList FREEframelist;
int frameOWNER[NUM_FRAMES]; // Track frame owned by which process or page
char physical_memory[PHYSICAL_MEMORY_SIZE]; // Physical memory simulation
int tickCURRENT = 0;

void loadPhysicalMemory(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fread(physical_memory, 1, PHYSICAL_MEMORY_SIZE, file);
    fclose(file);
}

void processINIT() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < PAGE_TABLE_SIZE; j++) {
            process[i].page_table[j].valid = false;
        }
    }
    FREEframelist.size = NUM_FRAMES;
    for (int i = 0; i < NUM_FRAMES; i++) {
        FREEframelist.frames[i] = i;
        frameOWNER[i] = -1;
    }
}

int frameALLOCATE() {
    if (FREEframelist.size == 0) {
        return -1; // if no free frames available
    }
    int frame_number = FREEframelist.frames[--FREEframelist.size];
    return frame_number;
}

void frameFREE(int frame_number) {
    FREEframelist.frames[FREEframelist.size++] = frame_number;
    frameOWNER[frame_number] = -1;
}

int victimframeFIND() {
    int oldest_tick = tickCURRENT;
    int victim_frame = -1;
    for (int i = 0; i < NUM_FRAMES; i++) {
        for (int p = 0; p < NUM_PROCESSES; p++) {
            for (int j = 0; j < PAGE_TABLE_SIZE; j++) {
                if (process[p].page_table[j].valid && process[p].page_table[j].frame_number == i) {
                    if (process[p].page_table[j].last_used < oldest_tick) {
                        oldest_tick = process[p].page_table[j].last_used;
                        victim_frame = i;
                    }
                }
            }
        }
    }
    return victim_frame;
}

void save(int pid, unsigned int page_number, int frame_number) {
    printf("    -> Saving Page %u of Process %d to disk (evicting Frame %d).\n", page_number, pid, frame_number);
}

void frametoprocessALLOCATE(int pid, unsigned int page_number, int frame_number) {
    process[pid].page_table[page_number].frame_number = frame_number;
    process[pid].page_table[page_number].valid = true;
    process[pid].page_table[page_number].last_used = tickCURRENT;
    frameOWNER[frame_number] = pid;
    printf("    -> Page Table Updated: Process %d, VA Page %u -> Frame %d\n", pid, page_number, frame_number);
}

void pageSWAP(int pid, unsigned int new_page_number) {
    int frameVICTIM = victimframeFIND();
    int victimPID = frameOWNER[frameVICTIM];
    unsigned int pagenumberVICTIM = -1;

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (process[victimPID].page_table[i].valid && process[victimPID].page_table[i].frame_number == frameVICTIM) {
            pagenumberVICTIM = i;
            process[victimPID].page_table[i].valid = false;
            break;
        }
    }
    printf("    -> Swapping out Page no. %u of Process %d (Frame %d).\n", pagenumberVICTIM, victimPID, frameVICTIM);
    save(victimPID, pagenumberVICTIM, frameVICTIM);
    frametoprocessALLOCATE(pid, new_page_number, frameVICTIM);
}

void memoryHANDLE(int pid, unsigned int va) {
    unsigned int numberPAGE = va / PAGE_SIZE;
    unsigned int offset = va % PAGE_SIZE;

    printf("[Tick %d] Process %d accessed VA 0x%X\n", tickCURRENT, pid, va);
    printf("  -> VA Breakdown: Page Number = 0x%X, Offset = 0x%X\n", numberPAGE, offset);

    if (process[pid].page_table[numberPAGE].valid) {
        unsigned int frame_number = process[pid].page_table[numberPAGE].frame_number;
        unsigned int pa = (frame_number * PAGE_SIZE) + offset;
        printf("  -> PA Translation Successful ! : VA 0x%X -> PA 0x%X\n", va, pa);
        printf("  -> Data at PA 0x%X: %c\n", pa, physical_memory[pa]);
        process[pid].page_table[numberPAGE].last_used = tickCURRENT;
    } else {
        printf("  -> No valid mapping in the page table. PAGE FAULT !\n");

        if (FREEframelist.size == 0) {
            printf("  -> Memory Full! Swapping required.\n");
            pageSWAP(pid, numberPAGE);
        } else {
            int frame_number = frameALLOCATE();
            frametoprocessALLOCATE(pid, numberPAGE, frame_number);
            unsigned int pa = (frame_number * PAGE_SIZE) + offset;
            printf("  -> New Frame Allocating : Frame  = %d\n", frame_number);
            printf("  -> PA Translation Successful ! : VA 0x%X -> PA 0x%X\n", va, pa);
        }
    }
}

void memorySIMULATE() {
    srand(time(NULL));
    for (tickCURRENT = 0; tickCURRENT < 50; tickCURRENT++) {
        int pid = rand() % NUM_PROCESSES;
        unsigned int va = rand() % (PAGE_TABLE_SIZE * PAGE_SIZE);
        memoryHANDLE(pid, va);
    }
}

int main() {
    loadPhysicalMemory("simple.bin");
    processINIT();
    memorySIMULATE();
    return 0;
}
