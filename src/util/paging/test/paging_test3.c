// paging_test3.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../paging.h"
#include "../../tlb/tlb.h"
#include "../../frame_list/frame_list.h"

// Swap functionality excluded

//extern uint8_t physical_memory[NUM_PHYSICAL_PAGES * PAGE_SIZE];

int main() {
    // Initialize physical memory
    init_physical_memory();

    // Initialize TLB
    tlb_t tlb;
    initialize_tlb(&tlb);
    tlb.policy = LRU; // Choose TLB replacement policy

    // Create page table
    u_pt_t* u_pt = create_u_pt();
    if (u_pt == NULL) {
        printf("Failed to create page table\n");
        return -1;
    }

    // Map virtual address
    uint16_t vaddr = 0x1234;
    if (map_page(u_pt, vaddr) != 0) {
        printf("Failed to map virtual address 0x%04X\n", vaddr);
        free_page_table(u_pt);
        return -1;
    }

    // Translate virtual address to physical address
    uint32_t paddr = 0;
    int res = translate_address(u_pt, &tlb, vaddr, &paddr);
    if (res == PAGE_FAULT) {
        printf("Failed to translate virtual address 0x%04X\n", vaddr);
        free_page_table(u_pt);
        return -1;
    }

    printf("Virtual address 0x%04X translated to physical address 0x%08X\n", vaddr, paddr);

    // Write data to virtual address
    const char *data = "Hello, Physical Memory via Virtual Address!";
    size_t data_len = strlen(data) + 1; // Include null terminator

    // Ensure data fits within page boundary
    uint16_t offset = vaddr & 0xFF;
    if (offset + data_len > PAGE_SIZE) {
        data_len = PAGE_SIZE - offset;
    }

    // Write data byte by byte via virtual addresses
    for (size_t i = 0; i < data_len; i++) {
        uint16_t current_vaddr = vaddr + i;
        uint32_t current_paddr = 0;
        res = translate_address(u_pt, &tlb, current_vaddr, &current_paddr);
        if (res == PAGE_FAULT) {
            printf("Failed to translate virtual address 0x%04X\n", current_vaddr);
            free_page_table(u_pt);
            return -1;
        }
        physical_memory[current_paddr] = data[i];
    }

    // Read back the data via virtual addresses
    char read_buffer[PAGE_SIZE];
    for (size_t i = 0; i < data_len; i++) {
        uint16_t current_vaddr = vaddr + i;
        uint32_t current_paddr = 0;
        res = translate_address(u_pt, &tlb, current_vaddr, &current_paddr);
        if (res == PAGE_FAULT) {
            printf("Failed to translate virtual address 0x%04X\n", current_vaddr);
            free_page_table(u_pt);
            return -1;
        }
        read_buffer[i] = physical_memory[current_paddr];
    }
    read_buffer[data_len] = '\0';

    printf("Data read from virtual address 0x%04X: %s\n", vaddr, read_buffer);

    // Clean up
    free_page_table(u_pt);

    return 0;
}