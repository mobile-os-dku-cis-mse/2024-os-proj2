#include "replacement_policy.h"

FIFOQueue fifo_queue;

void initialize_fifo_queue() {
    fifo_queue.front = -1;
    fifo_queue.rear = -1;
    fifo_queue.size = 0;
}

int is_fifo_empty() {
    return fifo_queue.size == 0;
}

int is_fifo_full() {
    return fifo_queue.size == FIFO_CAPACITY;
}

void fifo_enqueue(int dir_idx, int table_idx){
    if (is_fifo_full()){
        fifo_queue.front = (fifo_queue.front + 1) % FIFO_CAPACITY;
        fifo_queue.size--;
    }

    if (fifo_queue.front == -1) fifo_queue.front = 0;

    fifo_queue.rear = (fifo_queue.rear + 1) % FIFO_CAPACITY;
    fifo_queue.dir_index[fifo_queue.rear] = dir_idx;
    fifo_queue.table_index[fifo_queue.rear] = table_idx;
    fifo_queue.size++;
}
void fifo_dequeue(int *dir_idx, int *table_idx){
    if (is_fifo_empty()) return;

    *dir_idx = fifo_queue.dir_index[fifo_queue.front];
    *table_idx = fifo_queue.table_index[fifo_queue.front];
    fifo_queue.front = (fifo_queue.front + 1) % FIFO_CAPACITY;
    fifo_queue.size--;

    if (fifo_queue.size == 0){
        fifo_queue.front = -1;
        fifo_queue.rear = -1;
    }
}

void lru_replacement(PageDirectory *page_directory, int *replace_dir_idx, int *replace_table_idx){
    int oldest_time = INT_MAX;
    bool page_found = false;

    for(int dir_idx = 0; dir_idx < 2; dir_idx++){
        PageTable *page_table = page_directory->page_table[dir_idx];
        if(page_table == NULL) continue;

        for(int table_idx = 0; table_idx < PAGE_NUM; table_idx++){
            if(page_table->valid[table_idx]){
                if(page_table->last_used[table_idx] < oldest_time){
                    oldest_time = page_table->last_used[table_idx];
                    *replace_dir_idx = dir_idx;
                    *replace_table_idx = table_idx;
                    page_found = true;
                }
            }
        }
    }

    if(!page_found){
        *replace_dir_idx = -1;
        *replace_table_idx = -1;
    }
}