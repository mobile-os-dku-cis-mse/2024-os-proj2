#include "tlb.h"

// TLB 초기화 함수
TLB* init_tlb(int size) {
    // TLB 메모리 할당
    TLB* tlb = (TLB* )malloc(sizeof(TLB));
    if (tlb == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for TLB structure.\n");
        exit(EXIT_FAILURE);
    }

    // TLB 엔트리 배열 할당
    tlb->entries = (TLBEntry*)malloc(sizeof(TLBEntry) * size);
    if (tlb->entries == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for TLB entries.\n");
        free(tlb); // 할당된 TLB 구조체 메모리 해제
        exit(EXIT_FAILURE);
    }

    // 엔트리 초기화
    for (int i = 0; i < size; i++) {
        tlb->entries[i].page_number = 0;
        tlb->entries[i].frame_number = 0;
        tlb->entries[i].valid = 0; // 유효하지 않은 상태로 초기화
        tlb->entries[i].last_used = 0;
        tlb->entries[i].access_count = 0;
        tlb->entries[i].read_only = 0;
    }

    tlb->size = size;
    tlb->current_time = 0; // 현재 시간 초기화

    return tlb;
}

// TLB 해제 함수
void free_tlb(TLB* tlb) {
    if (tlb != NULL) {
        // 엔트리 배열 해제
        if (tlb->entries != NULL) {
            free(tlb->entries);
        }

        // TLB 구조체 해제
        free(tlb);
    }
}
