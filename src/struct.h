#ifndef STRUCT_H
#define STRUCT_H

#include <stdbool.h>
#include <stdio.h>

#define PAGE_SIZE 4096
#define PHYSICAL_MEMORY_SIZE (16 * 1024 * 1024)
#define NUM_PROCESSES 10
#define MAX_TICKS 10000
#define MAX_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)

typedef struct PageTableEntry {
    int frame_number;
    bool valid;
    bool swapped;
    bool modified;
} PageTableEntry;

typedef struct Process {
    int pid;
    PageTableEntry **first_level_table;
} Process;

typedef struct Kernel {
    Process *process_list[NUM_PROCESSES + 1];
    int *free_frames;
    int free_frame_count;
    int disk_store[MAX_FRAMES];
    int current_tick;
    FILE *log_file;
} Kernel;

#endif //STRUCT_H
