
// pcb_queue.c
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "pid_queue.h"
//#define DEBUG

extern FILE* pcb_logs;
pcb_t* create_pcb(pid_t pid, int cpu_burst, int priority) {
    pcb_t* new_pcb = (pcb_t*)malloc(sizeof(pcb_t));
    if (!new_pcb) {
        perror("Failed to allocate memory for PCB");
        exit(EXIT_FAILURE);
    }
    
    new_pcb->pid = pid;
    new_pcb->cpu_burst_time = cpu_burst;
    new_pcb->io_burst_time = 0;
    new_pcb->remaining_time = 0;
    new_pcb->priority = priority;
    new_pcb->state = PROCESS_READY;
    new_pcb->arrival_time = time(NULL);
    new_pcb->start_time = 0;
    new_pcb->completion_time = 0;
    new_pcb->next = NULL;
    
    return new_pcb;
}

void destroy_pcb(pcb_t* pcb) {
    if (pcb) {
        free(pcb);
    }
}

pcb_queue* create_pcb_queue() {
    pcb_queue* queue = (pcb_queue*)malloc(sizeof(pcb_queue));
    if (!queue) {
        perror("Failed to allocate memory for queue");
        exit(EXIT_FAILURE);
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

void enqueue_pcb(pcb_queue* queue, pcb_t* process) {
    if (!process) return;
    
    process->next = NULL;
    
    if (is_queue_empty(queue)) {
        queue->front = process;
    } else {
        queue->rear->next = process;
    }
    
    queue->rear = process;
    queue->size++;
}

pcb_t* dequeue_pcb(pcb_queue* queue, char* callback_name) {
    if (is_queue_empty(queue)) {
        return NULL;
    }
    
    pcb_t* temp = queue->front;
    queue->front = queue->front->next;
    
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    
    temp->next = NULL;  // 분리된 PCB의 next 포인터 초기화
    queue->size--;
#ifdef DEBUG
    printf("[PID_QUEUE] current return pcb is %d, call by %s\n", temp->pid, callback_name);
#endif

    return temp;
}

void remove_pcb(pcb_queue* queue, pid_t pid) {
    if (is_queue_empty(queue)) {
        return;
    }
    
    pcb_t* current = queue->front;
    pcb_t* prev = NULL;
    
    while (current != NULL) {
        if (current->pid == pid) {
            if (prev == NULL) {
                queue->front = current->next;
                if (queue->front == NULL) {
                    queue->rear = NULL;
                }
            } else {
                prev->next = current->next;
                if (current->next == NULL) {
                    queue->rear = prev;
                }
            }
            destroy_pcb(current);
            queue->size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

pcb_t* find_pcb(pcb_queue* queue, pid_t pid) {
    pcb_t* current = queue->front;
    while (current != NULL) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_pcb(pcb_t* pcb) {
    if (pcb == NULL) {
        printf("PCB is NULL\n");
        return;
    }

    fprintf(pcb_logs,"========================================\n");
    fprintf(pcb_logs,"Process ID          : %d\n", pcb->pid);
    fprintf(pcb_logs,"State               : %d\n", pcb->state);
    fprintf(pcb_logs,"Arrival Time        : %u\n", (unsigned int)pcb->arrival_time);
    fprintf(pcb_logs,"Start Time          : %u\n", (unsigned int)pcb->start_time);
    fprintf(pcb_logs,"Completion Time     : %u\n", (unsigned int)pcb->completion_time);
    fprintf(pcb_logs,"Waiting Time        : %u\n", pcb->waiting_time);
    fprintf(pcb_logs,"Response Time       : %u\n", pcb->response_time);
    fprintf(pcb_logs,"Execution Time      : %u\n", pcb->execution_time);
    fprintf(pcb_logs,"========================================\n");
}

// print its cpu burst and io burst
void print_ready_pcb_queue(pcb_queue* queue) {
    if (queue == NULL || is_queue_empty(queue)) {
        printf("Queue is empty.\n");
        return;
    }
    pcb_t* current = queue->front;
    printf("Ready Queue: ");
    while (current != NULL) {
        printf("[%d(%d:%d)] ", current->pid, current->cpu_burst_time, current->io_burst_time);
        current = current->next;
    }
    printf("\n");
}

void print_waiting_pcb_queue(pcb_queue* queue) {
    if (queue == NULL || is_queue_empty(queue)) {
        printf("Queue is empty.\n");
        return;
    }
    pcb_t* current = queue->front;
    printf("Waiting Queue: ");
    while (current != NULL) {
        printf("[%d(X:%d)] ", current->pid, current->io_time);
        current = current->next;
    }
    printf("\n\n");
}


void destroy_pcb_queue(pcb_queue* queue) {
    while (!is_queue_empty(queue)) {
        pcb_t* pcb = dequeue_pcb(queue, "destroy_pcb");
        destroy_pcb(pcb);
    }
    free(queue);
}

int is_queue_empty(pcb_queue* queue) {
    return queue->size == 0;
}

int get_queue_size(pcb_queue* queue) {
    if (queue == NULL) {
        return 0;
    }
    return queue->size;
}