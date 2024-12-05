#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmm2.h"

int find_free_swap_slot(void) {
    for (int i = 0; i < SWAP_SIZE; i++) {
        if (!swap_space[i].is_used) {
            return i;
        }
    }
    return -1;
}

int find_in_swap(int process_id, uint32_t page_number) {
    for (int i = 0; i < SWAP_SIZE; i++) {
        if (swap_space[i].is_used && 
            swap_space[i].process_id == process_id && 
            swap_space[i].page_number == page_number) {
            return i;
        }
    }
    return -1;
}

int swap_out_page(void) {
    update_swap_stats(0);
    int victim_frame = find_lru_victim();
    if (victim_frame == -1) return -1;

    int swap_slot = find_free_swap_slot();
    if (swap_slot == -1) {
        printf("[Swap] Error: No free swap slots\n");
        return -1;
    }

    FrameInfo *victim = &frames[victim_frame];
    swap_space[swap_slot].process_id = victim->process_id;
    swap_space[swap_slot].page_number = victim->virtual_page;
    swap_space[swap_slot].is_used = 1;
    
    memcpy(swap_space[swap_slot].data,
           get_frame_address(victim_frame),
           PAGE_SIZE);

    victim->is_used = 0;
    printf("[Swap] Swapped out page %u to slot %d\n", 
           victim->virtual_page, swap_slot);
    
    return victim_frame;
}

int swap_in_page(int process_id, uint32_t page_number) {
    update_swap_stats(1);
    int swap_index = find_in_swap(process_id, page_number);
    if (swap_index == -1) {
        printf("[Swap] Error: Page not found in swap\n");
        return -1;
    }

    int frame = find_lru_victim();
    if (frame == -1) {
        frame = swap_out_page();
        if (frame == -1) return -1;
    }

    memcpy(get_frame_address(frame),
           swap_space[swap_index].data,
           PAGE_SIZE);

    frames[frame].is_used = 1;
    frames[frame].process_id = process_id;
    frames[frame].virtual_page = page_number;
    frames[frame].last_access_time = ++global_clock;

    swap_space[swap_index].is_used = 0;
    
    printf("[Swap] Swapped in page %u to frame %d\n", 
           page_number, frame);
    
    return frame;
}

void print_memory_status(void) {
    printf("\n===== Memory Status =====\n");
    
    printf("+++ Frame 0 ~ 10240 은 Page Table을 위해 사용함 +++\n");
    // Print frame usage
    for (int i = 10241; i < NUM_FRAMES; i++) {
        if (frames[i].is_used) {
        printf("Frame %2d: ", i);
            printf("Process %d, Page %u, Last Access: %lu%s\n",
                   frames[i].process_id,
                   frames[i].virtual_page,
                   frames[i].last_access_time,
                   frames[i].is_page_table ? " (Page Table)" : "");
        } else {
            //printf("Free\n"); // 표시하면 너무 많아서 표시 안함
        }
    }

    // Print swap space usage
    int used_swap = 0;
    for (int i = 0; i < SWAP_SIZE; i++) {
        if (swap_space[i].is_used) used_swap++;
    }
    
    printf("\nSwap Space Usage: %d/%d pages\n", used_swap, SWAP_SIZE); // 값이 작을수록 좋은 상황
    //분모에 해당하는 스왑페이지 공간이 있고 그 중 분자에 해당하는 스왑공간에 저장된 페이지가 있음
    //현재 스왑공간에 저장된 페이지가0개 => 모든 필요한 페이지가 물리 메모리 내에서 처리 중
    //=> 디스크 I/O가 발생하지 않음
    printf("=====================\n\n");
}
