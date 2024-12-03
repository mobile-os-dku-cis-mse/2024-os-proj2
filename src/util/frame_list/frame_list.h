//
// Created by hochacha on 24. 12. 2.
//

#ifndef FRAME_LIST_H
#define FRAME_LIST_H

#define PHYSICAL_MEMORY_SIZE (16 * 1024) // 16KB
#define PAGE_SIZE 256
#define NUM_PHYSICAL_PAGES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)

typedef struct {
    int frame_number;
    int in_use;
} page_frame_t;

extern page_frame_t free_page_list[NUM_PHYSICAL_PAGES];

void init_physical_memory();
int allocate_page();
void free_page(int frame_number);

#endif //FRAME_LIST_H
