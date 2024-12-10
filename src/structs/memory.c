#include "memory.h"

char* init_memory(int size) {
    char* memory = (char*)malloc(sizeof(char) * size);
    if (memory == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for init_memory\n");
        return NULL;
    }
    for (int i = 0; i < size; i++) {
        memory[i] = 0;
    }
    return memory;
}

void free_memory(char* memory) {
    if (memory) {
        free(memory);
    }
}

char* init_disk() {
    char* disk = (char*)malloc(sizeof(char) * MAX_DISK_SIZE);
    if (disk == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for init_disk\n");
        return NULL;
    }
    for (int i = 0; i < MAX_DISK_SIZE; i++) {
        disk[i] = 0;
    }
    return disk;
}

void free_disk(char* disk) {
    if (disk) {
        free(disk);
    }
}

MemoryMap* init_memory_map(int size) {
    MemoryMap* memory_map = (MemoryMap*)malloc(sizeof(MemoryMap) * size);
    if (memory_map == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for init_memory_map\n");
        return NULL;
    }
    for (int i = 0; i < size; i++) {
        memory_map[i].is_valid = 0;
        memory_map[i].virtual_address = 0;
        memory_map[i].last_used = 0;
        memory_map[i].arrival_time = 0;
        memory_map[i].access_count = 0;
        memory_map[i].reference_bit = 0;
    }
    return memory_map;
}

void free_memory_map(MemoryMap* memory_map) {
    if (memory_map) {
        free(memory_map);
    }
}

DiskMap* init_disk_map(int size) {
    int disk_size = MAX_DISK_SIZE/size;
    DiskMap* disk_map = (DiskMap*)malloc(sizeof(DiskMap) * disk_size);
    if (disk_map == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for init_disk_map\n");
        return NULL;
    }
    for (int i = 0; i < disk_size; i++) {
        disk_map[i].is_valid = 0;
        disk_map[i].virtual_address = 0;
    }
    return disk_map;
}


void free_disk_map(DiskMap* disk_map) {
    if (disk_map) {
        free(disk_map);
    }
}
