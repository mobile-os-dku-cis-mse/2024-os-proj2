//
// Created by hochacha on 24. 12. 2.
//

#include "swap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../paging/paging.h"
#include "../frame_list/frame_list.h"

/*
    System Architecture

    Virttual Memory 
    - 16bit address space
      - 4bit upper index
      - 4bit lower index
      - 8bit offset
    - 256B page size
    - 256 page entries

    Physical Memory
    - 32bit address space
    - 16KB sized shared memory
    - 256B page size

    Swapping
    - 1MB File (let the file stores 4096 pages)
    - 256B page size

*/


swap_space_t swap_space;

// Function Prototypes
int write_page_to_swap(uint32_t swap_offset, int frame_number);
int read_page_from_swap(uint32_t swap_offset, int frame_number);

// Initialize swap space: create swap file and initialize bitmap
int init_swap_space() {
    swap_space.swap_file = fopen(SWAP_FILE_NAME, "w+b");
    if (swap_space.swap_file == NULL) {
        perror("Failed to create swap file");
        return SWAP_FAIL;
    }

    // Initialize swap bitmap to 0 (all slots free)
    memset(swap_space.swap_bitmap, 0, sizeof(swap_space.swap_bitmap));

    return SWAP_SUCCESS;
}

// Close swap space: close swap file
void close_swap_space() {
    if (swap_space.swap_file != NULL) {
        fclose(swap_space.swap_file);
        swap_space.swap_file = NULL;
    }
}

// Allocate a swap slot: returns slot index or NO_SWAP_SLOT if full
int allocate_swap_slot() {
    for (int i = 0; i < MAX_SWAP_PAGES; i++) {
        if (swap_space.swap_bitmap[i] == 0) {
            swap_space.swap_bitmap[i] = 1;
            return i;
        }
    }
    // Swap space is full
    return NO_SWAP_SLOT;
}

// Free a swap slot
void free_swap_slot(uint32_t slot) {
    if (slot >= 0 && slot < MAX_SWAP_PAGES) {
        swap_space.swap_bitmap[slot] = 0;
    }
}

// Write a page from physical memory to swap file at swap_offset
int write_page_to_swap(uint32_t swap_offset, int frame_number) {
    if (fseek(swap_space.swap_file, swap_offset, SEEK_SET) != 0) {
        perror("Failed to seek swap file for writing");
        return SWAP_FAIL;
    }

    uint8_t* page_data = &physical_memory[frame_number * PAGE_SIZE];
    size_t bytes_written = fwrite(page_data, 1, PAGE_SIZE, swap_space.swap_file);
    if (bytes_written != PAGE_SIZE) {
        perror("Failed to write page to swap file");
        return SWAP_FAIL;
    }

    fflush(swap_space.swap_file);
    return SWAP_SUCCESS;
}

// Read a page from swap file at swap_offset into physical memory at frame_number
int read_page_from_swap(uint32_t swap_offset, int frame_number) {
    if (fseek(swap_space.swap_file, swap_offset, SEEK_SET) != 0) {
        perror("Failed to seek swap file for reading");
        return SWAP_FAIL;
    }

    uint8_t* page_data = &physical_memory[frame_number * PAGE_SIZE];
    size_t bytes_read = fread(page_data, 1, PAGE_SIZE, swap_space.swap_file);
    if (bytes_read != PAGE_SIZE) {
        perror("Failed to read page from swap file");
        return SWAP_FAIL;
    }

    return SWAP_SUCCESS;
}

// Find the virtual address corresponding to a given frame number
uint16_t get_vaddr_from_frame(u_pt_t* u_pt, int frame_number) {
    for (int upper_index = 0; upper_index < NUM_UPPER_ENTRIES; upper_index++) {
        u_pte_t* u_pte = &u_pt->entries[upper_index];
        if (u_pte->present && u_pte->lower_table != NULL) {
            l_pt_t* l_pt = u_pte->lower_table;
            for (int lower_index = 0; lower_index < NUM_LOWER_ENTRIES; lower_index++) {
                l_pte_t* l_pte = &l_pt->entries[lower_index];
                if (l_pte->present && l_pte->frame_number == frame_number) {
                    uint16_t vaddr = (upper_index << 12) | (lower_index << 8);
                    return vaddr;
                }
            }
        }
    }
    // If not found, return an invalid virtual address
    return 0xFFFF;
}

// Swap out a page: writes the page to swap and updates the page table
int swap_out_page(u_pt_t* u_pt, uint32_t victim_frame_number) {
    // Find the virtual address that maps to the victim frame
    uint16_t victim_vaddr = get_vaddr_from_frame(u_pt, victim_frame_number);
    if (victim_vaddr == 0xFFFF) {
        fprintf(stderr, "Failed to find virtual address for frame %d\n", victim_frame_number);
        return SWAP_FAIL;
    }

    // Allocate a swap slot
    int slot = allocate_swap_slot();
    if (slot == NO_SWAP_SLOT) {
        fprintf(stderr, "Swap space is full. Cannot swap out frame %d\n", victim_frame_number);
        return SWAP_FAIL;
    }

    uint32_t swap_offset = slot * PAGE_SIZE;

    // Get the page table entry
    uint8_t upper_index = victim_vaddr >> 12;
    uint8_t lower_index = (victim_vaddr >> 8) & 0xF;
    l_pte_t* l_pte = &u_pt->entries[upper_index].lower_table->entries[lower_index];

    // Write the page to swap file
    if (write_page_to_swap(swap_offset, victim_frame_number) != SWAP_SUCCESS) {
        free_swap_slot(slot);
        return SWAP_FAIL;
    }

    // Update the page table entry
    l_pte->present = 0;
    l_pte->swapped = 1;
    l_pte->swap_offset = slot;

    // Free the physical frame
    free_frame(victim_frame_number);

    printf("Swapped out virtual address 0x%04X from frame %d to swap slot %d\n", victim_vaddr, victim_frame_number, slot);

    return SWAP_SUCCESS;
}
// Swap in a page: reads the page from swap and updates the page table
int swap_in_page(u_pt_t* u_pt, uint16_t vaddr) {
    uint8_t upper_index = vaddr >> 12;
    uint8_t lower_index = (vaddr >> 8) & 0xF;
    l_pte_t* l_pte = &u_pt->entries[upper_index].lower_table->entries[lower_index];

    if (!l_pte->swapped) {
        fprintf(stderr, "Page at virtual address 0x%04X is not swapped out\n", vaddr);
        return SWAP_FAIL;
    }

    // Allocate a physical frame
    int frame_number = allocate_frame();
    if (frame_number == -1) {
        // No free frames, need to swap out a victim page
        int victim_frame = select_victim_frame();
        if (victim_frame == -1) {
            fprintf(stderr, "No frames available to swap in and cannot select a victim.\n");
            return SWAP_FAIL;
        }

        // Swap out the victim frame
        if (swap_out_page(u_pt, victim_frame) != SWAP_SUCCESS) {
            fprintf(stderr, "Failed to swap out victim frame %d\n", victim_frame);
            return SWAP_FAIL;
        }

        // Now allocate the freed frame
        frame_number = allocate_frame();
        if (frame_number == -1) {
            fprintf(stderr, "Failed to allocate frame after swapping out victim.\n");
            return SWAP_FAIL;
        }
    }

    // Calculate swap offset from swap slot
    int swap_slot = l_pte->swap_offset;
    uint32_t swap_offset = swap_slot * PAGE_SIZE;

    // Read the page data from swap file into physical memory
    if (read_page_from_swap(swap_offset, frame_number) != SWAP_SUCCESS) {
        free_frame(frame_number);
        return SWAP_FAIL;
    }

    // Update the page table entry
    l_pte->frame_number = frame_number;
    l_pte->present = 1;
    l_pte->swapped = 0;
    l_pte->swap_offset = NO_SWAP_SLOT;

    // Free the swap slot
    free_swap_slot(swap_slot);

    printf("Swapped in virtual address 0x%04X to frame %d from swap slot %d\n", vaddr, frame_number, swap_slot);

    return SWAP_SUCCESS;
}