// paging_test_swap.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../paging.h"
#include "../../tlb/tlb.h"
#include "../../frame_list/frame_list.h"
#include "../../swap/swap.h"

extern uint8_t physical_memory[NUM_PHYSICAL_PAGES * PAGE_SIZE];

int main() {
    // Initialize physical memory and swap space
    init_physical_memory();
    if (init_swap_space() != SWAP_SUCCESS) {
        printf("Failed to initialize swap space\n");
        return -1;
    }

    // Initialize TLB
    tlb_t tlb;
    initialize_tlb(&tlb);
    tlb.policy = LRU; // Choose TLB replacement policy

    // Create page table
    u_pt_t* u_pt = create_u_pt();
    if (u_pt == NULL) {
        printf("Failed to create page table\n");
        close_swap_space();
        return -1;
    }

    // Define number of virtual pages to exceed physical memory and trigger swap
    const int num_virtual_pages = NUM_PHYSICAL_PAGES + 10;
    uint16_t base_vaddr = 0x0000;

    // Write data to each virtual page
    for (int i = 0; i < num_virtual_pages; i++) {
        if (i == 64) {
            printf("Reached 64th page.\n"); // 디버깅을 위한 출력
        }

        uint16_t vaddr = base_vaddr + (i * PAGE_SIZE);
        if (map_page(u_pt, vaddr) != 0) {
            printf("Failed to map virtual address 0x%04X\n", vaddr);
            free_page_table(u_pt);
            close_swap_space();
            return -1;
        }

        uint32_t paddr = 0;
        int res = translate_address(u_pt, &tlb, vaddr, &paddr);
        if (res == -1) { // PAGE_FAULT을 -1로 가정
            printf("Page fault at virtual address 0x%04X\n", vaddr);
            free_page_table(u_pt);
            close_swap_space();
            return -1;
        }

        // Write data to physical memory
        char data = 'A' + (i % 26); // Cycle through 'A' to 'Z'
        physical_memory[paddr] = data;

        printf("[%02d]Mapped VADDR 0x%04X to PADDR 0x%08X with data '%c'\n", i, vaddr, paddr, data);
    }

    printf("\n===== Data Writing Completed =====\n\n");

    // Read and verify data from each virtual page
    for (int i = 0; i < num_virtual_pages; i++) {
        uint16_t vaddr = base_vaddr + (i * PAGE_SIZE);
        uint32_t paddr = 0;
        int res = translate_address(u_pt, &tlb, vaddr, &paddr);
        if (res == -1) { // PAGE_FAULT을 -1로 가정
            printf("Page fault at virtual address 0x%04X during read\n", vaddr);
            free_page_table(u_pt);
            close_swap_space();
            return -1;
        }

        char data = physical_memory[paddr];
        printf("[%02d]Read from VADDR 0x%04X (PADDR 0x%08X): '%c'\n", i, vaddr, paddr, data);
    }

    printf("\n===== Data Reading Completed =====\n\n");

    // Clean up
    free_page_table(u_pt);
    close_swap_space();

    return 0;
}