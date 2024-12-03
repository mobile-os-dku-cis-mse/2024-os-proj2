#pragma once

#include "common.h"

typedef struct Proc Process;
typedef struct PT PageTable;
typedef struct PD PageDirectory;

typedef struct Node{
    Process *pcb;
    struct Node* next;
} Node;

typedef struct{
    Node* head;
    Node* tail;
    int count;
} Queue;

Queue* createQueue();
int isEmpty(Queue *q);
void enqueue(Queue *q, int pid, int original_cpu_burst, int original_io_burst, int cpu_burst, int io_burst, double arrival_time, int remaining_time, PageDirectory *page_directory);
Process* dequeue(Queue *q);
void sort_queue(Queue *q);
void removeQueue(Queue *q);