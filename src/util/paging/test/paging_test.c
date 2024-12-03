#include <stdio.h>
#include "../../tlb/tlb.h"
#include "../../frame_list/frame_list.h"
#include "../paging.h"

int main() {
    // 물리 메모리 초기화
    init_physical_memory();

    // TLB 초기화
    tlb_t tlb;
    initialize_tlb(&tlb);

    // 상위 페이지 테이블 생성
    u_pt_t* u_pt = create_u_pt();
    if (u_pt == NULL) {
        perror("Failed to create upper-level page table");
        return -1;
    }

    // 예시 가상 주소
    uint16_t vaddr = 0x1234;

    // 페이지 매핑
    if (map_page(u_pt, vaddr) != 0) {
        printf("Failed to map virtual address 0x%04X\n", vaddr);
        free_page_table(u_pt);
        return -1;
    }

    // 주소 변환
    uint32_t paddr = 0;
    if (translate_address(u_pt, &tlb, vaddr, &paddr) != 0) {
        printf("Failed to translate virtual address 0x%04X\n", vaddr);
    } else {
        printf("Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr, paddr);
    }

    // 페이지 언매핑
    unmap_page(u_pt, &tlb, vaddr);

    // 페이지 테이블 해제
    free_page_table(u_pt);
}