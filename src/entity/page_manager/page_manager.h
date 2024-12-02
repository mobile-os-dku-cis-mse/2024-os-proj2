//
// Created by hochacha on 11/25/24.
//

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_ENTRIES 1024
#define FRAME_COUNT 1024
#define OFFSET_BITS 12
#define TABLE_INDEX_BITS 10


// 각 메모리 별로, 실제 사용 가능한 페이지는 20개 (프로세스 10개, 전체 Free page 할당은 250개)
// 이후, 페이지 접근 방식에 따라서 분석할 것
// page_manager 함수가 계속 돌고 있을 것이지만, 결국에는 공유 변수인, PTBR이랑 current_process를 건드는 과정에서
// 동기화 Error가 발생할 것임



// PTE
typedef struct{
    unsigned int present:1;
    unsigned int writeable:1;
    unsigned int user:1;
    unsigned int accessed:1;
    unsigned int dirty:1;
    unsigned int unused:7;
    unsigned int frame:20;
}pte_t;

typedef struct{
    unsigned char bitmap[FRAME_COUNT/8];
    int free_frames;
}frame_manager_t;

typedef union{
    unsigned int value;
    struct{
        unsigned int offset:OFFSET_BITS;
        unsigned int second_table:TABLE_INDEX_BITS;
        unsigned int first_tables:TABLE_INDEX_BITS;
    }parts;
}vaddr_t;

frame_manager_t frame_manager;
pte_t* page_directory = NULL;

#endif //PAGE_MANAGER_H
