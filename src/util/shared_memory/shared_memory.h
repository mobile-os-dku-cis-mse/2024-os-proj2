//
// Created by hochacha on 11/25/24.
//
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H



// 상수 정의
#define SHARED_MEM_SIZE 0x400000  // 4MB
#define PAGE_SIZE 4096            // 4KB
#define NUM_PAGES (SHARED_MEM_SIZE / PAGE_SIZE)
#define SHM_NAME "/physical_memory"

typedef struct {
    unsigned char physical_memory[SHARED_MEM_SIZE];
} SharedMemory;

extern SharedMemory* p_mem;
SharedMemory* init_shared_memory();
void cleanup_shared_memory(SharedMemory* shm);
#endif //SHARED_MEMORY_H
