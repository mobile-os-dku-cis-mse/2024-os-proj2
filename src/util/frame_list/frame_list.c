//
// Created by hochacha on 24. 12. 2.
//

#include "frame_list.h"

page_frame_t free_page_list[NUM_PHYSICAL_PAGES];

void init_physical_memory() {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        free_page_list[i].frame_number = i;
        free_page_list[i].in_use = 0;
    }
}

int allocate_page() {
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        if (!free_page_list[i].in_use) {
            free_page_list[i].in_use = 1;
            return free_page_list[i].frame_number;
        }
    }
    return -1; // No free pages available
}

void free_page(int frame_number) {
    if (frame_number >= 0 && frame_number < NUM_PHYSICAL_PAGES) {
        free_page_list[frame_number].in_use = 0;
    }
}
 