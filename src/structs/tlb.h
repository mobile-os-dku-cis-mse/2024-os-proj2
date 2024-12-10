#ifndef TLB_H
#define TLB_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// TLB 엔트리 구조체
typedef struct {
    uint32_t page_number;   // 가상 페이지 번호
    uint32_t frame_number;  // 물리 프레임 번호
    uint8_t valid;          // 유효 비트
    uint32_t last_used;     // 마지막 사용 시간 (LRU를 위한)
    uint32_t access_count;  // 접근 횟수 (LFU를 위한)
    uint8_t read_only;     // 읽기 전용 플래그
} TLBEntry;

// TLB 구조체
typedef struct {
    TLBEntry* entries;      // TLB 엔트리 배열
    int size;               // TLB 크기
    uint32_t current_time;  // 현재 시간 (틱 단위)
    int fifo_index;         // FIFO 정책을 위한 순환 인덱스
} TLB;

TLB* init_tlb(int size);
void free_tlb(TLB* tlb);

#endif
