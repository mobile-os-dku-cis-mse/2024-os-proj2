#ifndef MEMORY_H
#define MEMORY_H


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_PHYSICAL_MEMORY_SIZE (128 * 1024 * 1024) // 최대 물리 메모리 크기 (128MB)
#define MAX_DISK_SIZE (1024 * 1024 * 1024)     // 디스크 크기 (1GB)


// mapping 관리하는 구조체
typedef struct {
    uint32_t virtual_address; // 해당 Frame이 매핑된 가상 주소
    int is_valid;             // 0: 비어 있음, 1: 매핑됨
    uint32_t last_used;        // 마지막 사용 시간 (LRU를 위한)
    uint32_t arrival_time;    // FIFO를 위한 도착 시간
    uint32_t access_count;     // 접근 횟수 (LFU를 위한)
    uint8_t reference_bit;     // 참조 비트 (SECOND_CHANCE를 위한)
} MemoryMap;


typedef struct 
{
    uint32_t virtual_address; // 해당 Frame이 매핑된 가상 주소
    int is_valid;       // 0: 비어 있음, 1: 매핑됨
} DiskMap;


char* init_memory(int size);
void free_memory(char* memory);

char* init_disk();
void free_disk(char* disk);

MemoryMap* init_memory_map(int size);
void free_memory_map(MemoryMap* memory_map);

DiskMap* init_disk_map();
void free_disk_map(DiskMap* disk_map);



#endif