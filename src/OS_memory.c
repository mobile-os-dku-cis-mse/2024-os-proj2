#include "OS_memory.h"

int initialize_os(int *free_frames, int total_frames)
{
    for (int i = 0; i < total_frames; i++) {
        free_frames[i] = i; // Tous les cadres sont initialement libres
    }
    return total_frames; // Retourner le nombre de cadres disponibles
}

void initialize_page_table(PageTable *page_table)
{
    for (int i = 0; i < NUM_FRAMES; i++) {
        page_table->entries[i].valid = false; // Aucune page mappée au départ
    }
}

void initialize_all_page_tables(PageTable page_tables[NUM_PROCESSES])
{
    for (int i = 0; i < NUM_PROCESSES; i++) {
        initialize_page_table(&page_tables[i]);
    }
}
