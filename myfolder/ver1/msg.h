// msg.h
#ifndef MSG_H
#define MSG_H

#define QUEUE_SIZE 10
#define PAGE_TABLE_SIZE 16    // 4비트 페이지 번호
#define FRAME_SIZE 4096      // 4KB
#define NUM_FRAMES 16        // 64KB / 4KB = 16
#define MAX_VIRT_ADDR 65536  // 16비트 주소 공간

// 페이지 테이블 엔트리 구조체
typedef struct page_table_entry {
    unsigned int frame_number:4;    // 16개 프레임
    unsigned int valid_bit:1;       // 유효 비트
    unsigned int dirty_bit:1;       // 변경 비트
    unsigned int referenced_bit:1;   // 참조 비트
    unsigned int protection_bits:3;  // 접근 권한
} page_table_entry;

// 페이지 테이블 구조체
typedef struct {
    page_table_entry entries[PAGE_TABLE_SIZE];
} PageTable;

// 메시지 버퍼 구조체 (프로세스 생성 시 사용)
typedef struct msgbuf {
    long mtype;       
    int pid;
    int idx;
    int io_burst;
    int cpu_burst;
} msgbuf;

// 메모리 접근 요청을 위한 메시지 구조체
typedef struct mem_msgbuf {
    long mtype;
    int pid;
    int process_index;
    unsigned int virtual_addresses[10];  // 한 번에 10개의 메모리 접근
} mem_msgbuf;

// 프로세스 구조체
typedef struct Process {
    pid_t pid;
    int idx;
    int cpu_burst;
    int io_burst;
    int remain_cpu_burst;
    int remain_io_burst;
} Process;

// 큐 구조체
typedef struct Queue {
    Process processes[QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

// 함수 선언
void initQueue(Queue* q);
int isFull(Queue* q);
int isEmpty(Queue* q);
void enqueue(Queue* q, Process* process);
Process* dequeue(Queue* q);
Process* getProcessAtIndex(Queue* q, int index);
void print_qstate(int rtime, Queue* runq, Queue* waitq);
void writeToFile(FILE *file, int rtime, int count, Queue* runq, Queue* waitq);
void waitq_burst();

#endif