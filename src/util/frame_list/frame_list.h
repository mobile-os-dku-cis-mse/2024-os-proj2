//
// Created by hochacha on 24. 12. 2.
//

#ifndef FRAME_LIST_H
#define FRAME_LIST_H

#include <stdint.h>

#define PHYSICAL_MEMORY_SIZE (16 * 1024) // 16KB
#define PAGE_SIZE 256
#define NUM_PHYSICAL_PAGES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)

typedef struct {
    uint32_t frame_number;
    int in_use;
    uint32_t last_used;
} page_frame_t;

extern page_frame_t free_page_list[NUM_PHYSICAL_PAGES];
extern uint8_t physical_memory[PHYSICAL_MEMORY_SIZE];
void init_physical_memory();
int allocate_frame();
uint32_t get_currrent_time();
void free_frame(int frame_number);
int select_victim_frame();
#endif //FRAME_LIST_H
