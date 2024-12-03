#pragma once

#include "common.h"
#include "process.h"
#include "queue.h"
#include "replacement_policy.h"

#define PAGE_SIZE 4096
#define PHYSICAL_SIZE 0x400000
#define VIRTUAL_SIZE 0x800000
#define FRAME_NUM (PHYSICAL_SIZE / PAGE_SIZE)
#define PAGE_NUM (VIRTUAL_SIZE / PAGE_SIZE)

#define FAULT_CODE_2 2
#define FAULT_CODE_1 1 
#define FAULT_CODE_0 0 

typedef struct PT{
    int *matching_frame;
    bool *valid;
    bool *isRead;
    bool *isWrite;

    int *last_used;
} PageTable;

typedef struct PD{
    PageTable **page_table;
} PageDirectory;

typedef struct FFL{
    int *free_frame_list;    
} FreeFrameList;

void initialize_memory(int **physical_memory, int **virtual_memory);
PageDirectory initialize_page_directory();
FreeFrameList initialize_free_frame_list();

int MMU(PageDirectory *page_directory, int virtual_address, bool *first_table_fault, bool *second_table_fault, bool *isRead, bool *isWrite);
int handle_page_fault(PageDirectory *page_directory, FreeFrameList *free_list, int virtual_address, int pid);
int copy_page(PageDirectory *page_directory, int virtual_address, FreeFrameList *free_list, int pid);
int swapping(PageDirectory *page_directory, FreeFrameList *free_page_list, int virtual_address, int pid);

void find_lru_page(PageDirectory *page_directory, int *replace_dir_idx, int *replace_table_idx);
void find_fifo_page(int *replace_dir_idx, int *replace_table_idx);

void write_page_to_disk(PageDirectory *page_directory, FreeFrameList *free_list, int pid, int dir_idx, int table_idx, int *data);
int find_from_disk(int pid, int dir_idx, int table_idx);
int read_page_from_disk(int line_num);

void demand_paging(FreeFrameList *free_frame_list, Queue * queue);