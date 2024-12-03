#pragma once

#include "common.h"
#include "virtual_memory.h"

#define FIFO_CAPACITY 1024

typedef struct{
    int dir_index[FIFO_CAPACITY];
    int table_index[FIFO_CAPACITY];
    int front;
    int rear;
    int size;
} FIFOQueue;

/* FIFO Replacement */
void initialize_fifo_queue();
int is_fifo_empty();
int is_fifo_full();
void fifo_enqueue(int dir_idx, int table_idx);
void fifo_dequeue(int *dir_idx, int *table_idx);

/* LRU Replacement*/
void lru_replacement(PageDirectory *page_directory, int *replace_dir_idx, int *replace_table_idx);