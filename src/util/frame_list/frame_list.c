//
// Created by hochacha on 24. 12. 2.
//

#include "frame_list.h"
#include <stdint.h>

page_frame_t free_page_list[NUM_PHYSICAL_PAGES];
uint8_t physical_memory[PHYSICAL_MEMORY_SIZE];

void init_physical_memory() {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        free_page_list[i].frame_number = i;
        free_page_list[i].in_use = 0;
        free_page_list[i].last_used = 0; 
    }
}

uint32_t allocate_frame(pid_t pid, uint16_t vaddr) {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (!free_page_list[i].in_use) {
            free_page_list[i].in_use = 1;
            free_page_list[i].pid = pid;    
            free_page_list[i].vaddr = vaddr;
            free_page_list[i].last_used = get_currrent_time();
            return free_page_list[i].frame_number;
        }
    }
    return -1; // No free pages available
}

void free_frame(int frame_number) {
    if (frame_number >= 0 && frame_number < NUM_PHYSICAL_PAGES) {
        free_page_list[frame_number].in_use = 0;
    }
}
uint32_t current_mem_time = 0;
uint32_t get_currrent_time(){
    return current_mem_time++;
}

uint32_t check_current_time(){
    return current_mem_time;
}


// return page number for eviction
int select_victim_frame() {
    uint32_t oldest_time = UINT32_MAX; 
    int victim_frame_number = -1;
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (free_page_list[i].in_use && free_page_list[i].last_used < oldest_time) {
            oldest_time = free_page_list[i].last_used;
            victim_frame_number = i;
        }
    }
    return victim_frame_number;
}

pid_t get_pid_from_frame(uint32_t victim_frame){
    return free_page_list[victim_frame].pid;
}