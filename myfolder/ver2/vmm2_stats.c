#include <stdio.h>
#include <time.h>
#include "vmm2.h"

typedef struct {
    uint64_t page_faults;
    uint64_t page_replacements;
    uint64_t swap_ins;
    uint64_t swap_outs;
    uint64_t memory_accesses;
    double avg_access_time;
    struct timespec total_time;
} VMStats;

static VMStats stats = {0};

void update_stats(void) {
    stats.memory_accesses++;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    stats.avg_access_time = 
        (stats.avg_access_time * (stats.memory_accesses - 1) + 
         (now.tv_nsec - stats.total_time.tv_nsec) / 1000000.0) / 
         stats.memory_accesses;
         
    stats.total_time = now;
}

// 새로운 통계 업데이트 함수들 추가
void update_memory_access(void) {
    update_stats();  // 기존 함수 호출
}

void update_page_fault(void) {
    stats.page_faults++;
}

void update_page_replacement(void) {
    stats.page_replacements++;
}

void update_swap_stats(int is_swap_in) {
    if (is_swap_in) {
        stats.swap_ins++;
    } else {
        stats.swap_outs++;
    }
}

void print_vm_stats(void) {
    printf("\n===== Virtual Memory Statistics =====\n");
    printf("Page Replacement Algorithm: %s\n", 
           current_algorithm == ALGORITHM_FIFO ? "FIFO" : "LRU");
    printf("Total Memory Accesses: %lu\n", stats.memory_accesses);
    printf("Page Faults: %lu (%.2f%%)\n", 
           stats.page_faults, 
           (double)stats.page_faults * 100 / stats.memory_accesses);
    printf("Page Replacements: %lu\n", stats.page_replacements);
    printf("Swap-ins: %lu\n", stats.swap_ins);
    printf("Swap-outs: %lu\n", stats.swap_outs);
    printf("Average Memory Access Time: %.2f ms\n", stats.avg_access_time);
    
    int used_frames = 0;
    int pt_frames = 0;
    for(int i = 0; i < NUM_FRAMES; i++) {
        if(frames[i].is_used) {
            used_frames++;
            if(frames[i].is_page_table) pt_frames++;
        }
    }
    
    printf("\nMemory Usage:\n");
    printf("Used Frames: %d/%d (%.1f%%)\n", 
           used_frames, NUM_FRAMES, 
           (float)used_frames * 100 / NUM_FRAMES);
    printf("Page Table Frames: %d (%.1f%%)\n", 
           pt_frames,
           (float)pt_frames * 100 / NUM_FRAMES);
    printf("================================\n\n");
}