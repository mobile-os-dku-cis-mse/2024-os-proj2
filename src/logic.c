#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logic.h"
#include "log.h"

Kernel* initialize_kernel() {
    Kernel *kernel = (Kernel *)malloc(sizeof(Kernel));
    kernel->free_frames = (int *)malloc(sizeof(int) * MAX_FRAMES);
    kernel->free_frame_count = MAX_FRAMES;
    kernel->current_tick = 0;
    memset(kernel->disk_store, -1, sizeof(kernel->disk_store));

    for (int i = 0; i < MAX_FRAMES; i++) {
        kernel->free_frames[i] = i;
    }

    kernel->log_file = fopen("simulation_logs.txt", "w");
    if (!kernel->log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    return kernel;
}

void initialize_processes(Kernel *kernel) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        Process *process = (Process *)malloc(sizeof(Process));
        process->pid = i;
        process->first_level_table = (PageTableEntry **)calloc(1024, sizeof(PageTableEntry *));
        kernel->process_list[i] = process;
    }
}

void fork_process(Kernel *kernel, int parent_pid) {
    Process *parent = kernel->process_list[parent_pid];
    if (!parent) {
        log_message(kernel, "Fork failed: Parent process not found.");
        return;
    }

    int child_pid = NUM_PROCESSES; // Next PID
    Process *child = (Process *)malloc(sizeof(Process));
    child->pid = child_pid;
    child->first_level_table = (PageTableEntry **)calloc(1024, sizeof(PageTableEntry *));

    for (int i = 0; i < 1024; i++) {
        if (parent->first_level_table[i]) {
            child->first_level_table[i] = parent->first_level_table[i];
        }
    }

    kernel->process_list[child_pid] = child;
    log_message(kernel, "Forked process.");
}

void swap_out(Kernel *kernel) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        Process *process = kernel->process_list[i];
        for (int j = 0; j < 1024; j++) {
            if (process->first_level_table[j]) {
                for (int k = 0; k < 1024; k++) {
                    PageTableEntry *entry = &process->first_level_table[j][k];
                    if (entry->valid) {
                        kernel->disk_store[entry->frame_number] = 1; // Mark as swapped
                        kernel->free_frames[kernel->free_frame_count++] = entry->frame_number;
                        entry->valid = false;
                        entry->swapped = true;
                        log_message(kernel, "Page swapped out.");
                        return;
                    }
                }
            }
        }
    }
}

void handle_page_fault(Kernel *kernel, Process *process, int first_index, int second_index) {
    if (kernel->free_frame_count == 0) swap_out(kernel);

    if (kernel->free_frame_count == 0) {
        log_message(kernel, "Critical error: No free frames even after swapping.");
        return;
    }

    int frame_number = kernel->free_frames[--kernel->free_frame_count];
    PageTableEntry *page_entry = &process->first_level_table[first_index][second_index];
    page_entry->frame_number = frame_number;
    page_entry->valid = true;
    log_message(kernel, "Page fault handled.");
}

void access_page(Kernel *kernel, Process *process, int virtual_address, bool is_write) {
    int first_index = (virtual_address / PAGE_SIZE) / 1024;
    int second_index = (virtual_address / PAGE_SIZE) % 1024;

    if (!process->first_level_table[first_index]) {
        process->first_level_table[first_index] = (PageTableEntry *)calloc(1024, sizeof(PageTableEntry));
    }

    PageTableEntry *page_entry = &process->first_level_table[first_index][second_index];

    if (page_entry->valid) {
        if (is_write && !page_entry->modified) {
            if (kernel->free_frame_count == 0) swap_out(kernel);
            int frame_number = kernel->free_frames[--kernel->free_frame_count];
            page_entry->frame_number = frame_number;
            page_entry->modified = true;
            log_message(kernel, "Copy-on-Write handled.");
        }
    } else {
        handle_page_fault(kernel, process, first_index, second_index);
    }
}

void simulate(Kernel *kernel) {
    for (kernel->current_tick = 0; kernel->current_tick < MAX_TICKS; kernel->current_tick++) {
        for (int i = 0; i < NUM_PROCESSES; i++) {
            Process *process = kernel->process_list[i];
            for (int j = 0; j < 10; j++) {
                int virtual_address = rand() % PHYSICAL_MEMORY_SIZE;
                bool is_write = rand() % 2;
                access_page(kernel, process, virtual_address, is_write);
            }
        }
    }
    log_message(kernel, "Simulation completed.");
}
