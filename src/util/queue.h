#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// PCB 구조체 정의
typedef struct PCB {
    pid_t pid;                // 프로세스 ID
    uint32_t va;            // 가상 주소
    char value;
    int read_write;         // 읽기/쓰기 플래그
} PCB;

// 일반 큐 노드 구조체 정의
typedef struct Node {
    PCB *pcb;               // 큐의 원소 (PCB 객체)
    struct Node *next;      // 다음 노드를 가리키는 포인터
} Node;

// 일반 큐 구조체 정의
typedef struct Queue {
    int count;              // 큐에 있는 원소의 개수
    Node *head;             // 큐의 첫 번째 노드
    Node *tail;             // 큐의 마지막 노드
} Queue;


// 일반 큐 함수 프로토타입
Queue* createQueue();                  // 큐 생성 함수
Node* createNode(PCB *pcb);            // 노드 생성 함수
void enqueue(Queue *queue, PCB *pcb);  // 큐에 원소 추가 함수
PCB* dequeue(Queue *queue);            // 큐에서 원소 제거 함수
void destroyQueue(Queue *queue);       // 큐 메모리 해제 함수

#endif // QUEUE_H
