#include "queue.h"

// 일반 큐 구현
Queue* createQueue() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    return queue;
}

Node* createNode(PCB *pcb) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->pcb = pcb;
    node->next = NULL;
    return node;
}

void enqueue(Queue *queue, PCB *pcb) {
    Node *node = createNode(pcb);
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->count++;
}

PCB* dequeue(Queue *queue) {
    if (queue->count == 0) return NULL;
    Node *node = queue->head;
    PCB *pcb = node->pcb;
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    free(node);
    queue->count--;
    return pcb;
}

void destroyQueue(Queue *queue) {
    while (queue->head) {
        Node *node = queue->head;
        queue->head = node->next;
        free(node);
    }
    free(queue);
}
