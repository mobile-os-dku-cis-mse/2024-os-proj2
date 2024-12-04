//
// Created by hochacha on 24. 12. 2.
//

#ifndef SWAP_H
#define SWAP_H

#include <stdio.h>
#include <stdint.h>
#include "../paging/paging.h"
#define SWAP_FILE_NAME "swapfile.bin"
#define PAGE_SIZE 256
#define MAX_SWAP_PAGES 1024  // 스왑 공간에 저장할 수 있는 최대 페이지 수

#define NO_SWAP_SLOT -1
#define SWAP_SUCCESS 0
#define SWAP_FAIL -1

typedef struct {
    FILE *swap_file;
    int swap_bitmap[MAX_SWAP_PAGES];  // 0: 사용 가능, 1: 사용 중
} swap_space_t;

int init_swap_space();
int allocate_swap_slot();
void close_swap_space();
void free_swap_slot(uint32_t slot);
int swap_out_page(u_pt_t* u_pt, uint32_t victim_frame_number, pid_t pid, uint16_t vaddr);
int swap_in_page(u_pt_t* u_pt, uint16_t vaddr, pid_t pid);
#endif //SWAP_H
