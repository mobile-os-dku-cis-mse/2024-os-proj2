#include <stdio.h>
#include "../../tlb/tlb.h"
#include "../../frame_list/frame_list.h"
#include "../paging.h"

void print_tlb_state(tlb_t* tlb) {
    printf("\nCurrent TLB State:\n");
    printf("Index\tValid\tVPN\t\tFrame\tR/W\tU/S\tTimestamp\tSCA\n");
    printf("-----------------------------------------------------\n");
    for (int i = 0; i < TLB_SIZE; i++) {
        printf("%5d\t%5d\t0x%08x\t0x%08x\t%8d\t%8d\t%8u\t\t%8d\n",
               i,
               tlb->entries[i].valid,
               tlb->entries[i].vpn,
               tlb->entries[i].frame_number,
               tlb->entries[i].read_write,
               tlb->entries[i].user_supervisor,
               tlb->entries[i].timestamp,
               tlb->entries[i].sca);
    }
    printf("\n");
}

int main() {
    // 물리 메모리 초기화
    init_physical_memory();

    // TLB 초기화
    tlb_t tlb = {0, };
    initialize_tlb(&tlb);

    // 상위 페이지 테이블 생성
    u_pt_t* u_pt = create_u_pt();
    if (u_pt == NULL) {
        perror("Failed to create upper-level page table");
        return -1;
    }

    // 테스트 케이스 1: 페이지 매핑 및 주소 변환
    uint16_t vaddr1 = 0x1234;
    if (map_page(u_pt, vaddr1) != 0) {
        printf("Failed to map virtual address 0x%04X\n", vaddr1);
        free_page_table(u_pt);
        return -1;
    }
    uint32_t paddr1 = 0;
    int res = translate_address(u_pt, &tlb, vaddr1, &paddr1);
    if (res == PAGE_FAULT) {
        printf("Failed to translate virtual address 0x%04X\n", vaddr1);
    } else if (res == TLB_HIT_PAGING){
        printf("[TEST PASSED: TLB_HIT] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr1, paddr1);
    } else {
        printf("[TEST PASSED: TLB_MISS] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr1, paddr1);
    }

    print_tlb_state(&tlb);

    // 테스트 케이스 2: 다른 가상 주소 매핑
    uint16_t vaddr2 = 0xABCD;
    if (map_page(u_pt, vaddr2) != 0) {
        printf("Failed to map virtual address 0x%04X\n", vaddr2);
        free_page_table(u_pt);
        return -1;
    }
    uint32_t paddr2 = 0;
    res = translate_address(u_pt, &tlb, vaddr2, &paddr2);
    if (res == PAGE_FAULT) {
        printf("Failed to translate virtual address 0x%04X\n", vaddr2);
    } else if (res == TLB_HIT_PAGING) {
        printf("[TEST PASSED: TLB_HIT] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr2, paddr2);
    } else {
        printf("[TEST PASSED: TLB_MISS] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr2, paddr2);
    }

    print_tlb_state(&tlb);

    res = translate_address(u_pt, &tlb, vaddr2, &paddr2);
    if (res == PAGE_FAULT) {
        printf("Failed to translate virtual address 0x%04X\n", vaddr2);
    } else if (res == TLB_HIT_PAGING) {
        printf("[TEST PASSED: TLB_HIT] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr2, paddr2);
    } else {
        printf("[TEST PASSED: TLB_MISS] Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr2, paddr2);
    }

    print_tlb_state(&tlb);

    // 테스트 케이스 3: 매핑하지 않은 가상 주소 변환 시도
    uint16_t vaddr3 = 0xFFFF;
    uint32_t paddr3 = 0;
    res = translate_address(u_pt, &tlb, vaddr3, &paddr3);
    if (res == PAGE_FAULT) {
        printf("[TEST PASSED] failed to translate unmapped virtual address 0x%04X\n", vaddr3);
    } else {
        printf("[TEST FAILED] Unexpectedly translated virtual address 0x%04X to physical address 0x%08X\n", vaddr3, paddr3);
    }

    print_tlb_state(&tlb);

    // 테스트 케이스 4: 페이지 언매핑 후 주소 변환 시도
    unmap_page(u_pt, &tlb, vaddr1);
    if (translate_address(u_pt, &tlb, vaddr1, &paddr1) == PAGE_FAULT) {
        printf("[TEST PASSED] failed to translate unmapped virtual address 0x%04X after unmapping\n", vaddr1);
    } else {
        printf("[TEST FAILED] Unexpectedly translated virtual address 0x%04X after unmapping to physical address 0x%08X\n", vaddr1, paddr1);
    }

    print_tlb_state(&tlb);

    // 테스트 케이스 5: 메모리 부족 상황 시뮬레이션
    // 모든 프레임을 할당하여 프레임 부족 상태 유발
    int allocation_failed = 0;
    for (int i = 0; i < NUM_PHYSICAL_PAGES; i++) {
        uint16_t temp_vaddr = 0x1000 + i * PAGE_SIZE;
        if (map_page(u_pt, temp_vaddr) != 0) {
            printf("[TEST PASSED] Failed to map virtual address 0x%04X - no free frames\n", temp_vaddr);
            allocation_failed = 1;
            break;
        }
    }

    print_tlb_state(&tlb);

    if (!allocation_failed) {
        // 추가 프레임 할당 시도
        if (map_page(u_pt, 0x2000) != 0) {
            printf("failed to map virtual address 0x2000 due to lack of free frames\n");
        } else {
            printf("Unexpectedly mapped virtual address 0x2000 despite expected lack of free frames\n");
        }
    }

    // 페이지 테이블 해제
    free_page_table(u_pt);

    return 0;
}