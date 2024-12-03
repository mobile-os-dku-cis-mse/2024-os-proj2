//
// Created by hochacha on 24. 12. 2.
//

#ifndef SWAP_H
#define SWAP_H

#include <stdio.h>

#define SWAP_FILE_NAME "swapfile.bin"
#define PAGE_SIZE 256
#define MAX_SWAP_PAGES 1024  // 스왑 공간에 저장할 수 있는 최대 페이지 수

typedef struct {
    FILE *swap_file;
    int swap_bitmap[MAX_SWAP_PAGES];  // 0: 사용 가능, 1: 사용 중
} swap_space_t;


#endif //SWAP_H
