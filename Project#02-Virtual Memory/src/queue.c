#include "queue.h"
#include "process.h"

Queue* createQueue(){
    Queue *q = (Queue*)malloc(sizeof(Queue));
    if(q == NULL){
        perror("Failed to create Queue");
        exit(EXIT_FAILURE);
    }

    q->head = q->tail = NULL;
    q->count = 0;
    return q;
}

int isEmpty(Queue *q){
    return (q->count == 0);
}

void enqueue(Queue *q, int pid, int original_cpu_burst, int original_io_burst, int cpu_burst, int io_burst, double arrival_time, int remaining_time, PageDirectory *page_directory){
    Node *node = (Node*)malloc(sizeof(Node));

    if(node == NULL){
        perror("Failed to enqueue process");
        exit(EXIT_FAILURE);
    }

    node->pcb = (Process*)malloc(sizeof(Process));
    if (node->pcb == NULL) {
        perror("Failed to allocate memory for PCB");
        free(node);
        exit(EXIT_FAILURE);
    }

    node->pcb->pid = pid;
    node->pcb->original_cpu_burst = original_cpu_burst;
    node->pcb->original_io_burst = original_io_burst;
    node->pcb->cpu_burst = cpu_burst;
    node->pcb->io_burst = io_burst;
    node->pcb->arrival_time = arrival_time;
    node->pcb->remaining_time = remaining_time;
    node->pcb->page_directory = page_directory;
    node->next = NULL;

    if(q->head == NULL){
        q->head = node;
        q->tail = node;
    }else{
        q->tail->next = node;
        q->tail = node;
    }
    q->count++;
}

Process* dequeue(Queue *q){
    if(isEmpty(q)){
        fprintf(stderr, "Queue is Empty\n");
        return NULL;
    }

    Node *temp = q->head;
    Process *process = (Process*)malloc(sizeof(Process));
    if(process == NULL){
        perror("Failed to allocate memory for process");
        exit(EXIT_FAILURE);
    }
    process = temp->pcb;

    q->head = q->head->next;  
    if(q->head == NULL){     
        q->tail = NULL;
    }

    free(temp);  
    q->count--;

    return process;
}

void sort_queue(Queue *q){
    if(q == NULL || q->head == NULL){
        return;
    }

    int swapped;
    Node *current;
    Node *last = NULL;

    do{
        swapped = 0;
        current = q->head;

        while(current->next != last){
            if(current->pcb->cpu_burst > current->next->pcb->cpu_burst){
                Process *temp = current->pcb;
                current->pcb = current->next->pcb;
                current->next->pcb = temp;
                swapped = 1;
            }
            current = current->next;
        }
        last = current;
    }while(swapped);
}

void removeQueue(Queue *q){
    while(!isEmpty(q)){
        Process *p = dequeue(q);
        free(p); 
    }
    free(q);
}