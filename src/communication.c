#include "OS_memory.h"

int translate_address(int *free_frames, int *free_frame_count, PageTable *page_table, int virtual_address)
{
    int vm_page_number = virtual_address / PAGE_SIZE;
    int vm_page_offset = virtual_address % PAGE_SIZE;

    // Vérifier si la page est valide
    if (page_table->entries[vm_page_number].valid) {
        int frame_number = page_table->entries[vm_page_number].frame_number;
        return frame_number * PAGE_SIZE + vm_page_offset;
    } else {
        // Faute de page : allouer un cadre libre
        if (*free_frame_count == 0) {
            printf("Erreur : Plus de cadres disponibles !\n");
            exit(EXIT_FAILURE);
        }
        int free_frame = free_frames[--(*free_frame_count)];
        page_table->entries[vm_page_number].frame_number = free_frame;
        page_table->entries[vm_page_number].valid = true;

        printf("Page fault: Allocated frame %d for virtual page %d\n", free_frame, vm_page_number);

        return free_frame * PAGE_SIZE + vm_page_offset;
    }
}

