// Ai Generated Code Acknowledgement
// pcb_queue.h
#ifndef PCB_QUEUE_H
#define PCB_QUEUE_H

#include <sys/types.h>
#include "../paging/paging.h"
// 프로세스 상태 정의
#define PROCESS_READY    0
#define PROCESS_RUNNING  1
#define PROCESS_BLOCKED  2
#define PROCESS_TERMINATED 3
#ifndef TIME_QUANTUAM
#define TIME_QUANTUAM 5
#endif



// PCB(Process Control Block) 구조체
typedef struct _PCB {
    pid_t pid;              // 프로세스 ID
    int cpu_burst_time;     // CPU 실행 시간
    int io_burst_time;      // I/O 작업 시간
    int remaining_time;     // 남은 타임 퀀텀
    int priority;           // 우선순위
    int state;              // 프로세스 상태
    unsigned int io_time;   // 입출력 요청 시간
    time_t arrival_time;    // 도착 시간
    time_t start_time;      // 첫 실행 시작 시간
    time_t completion_time; // 완료 시간
    struct _PCB* next;      // 다음 PCB 포인터

    // 추가된 메트릭 필드
    unsigned int waiting_time;      // 대기 시간: Ready 큐에서 보낸 총 시간
    unsigned int response_time;     // 응답 시간: 도착부터 첫 스케줄링까지의 시간
    unsigned int execution_time;    // 실행 시간: 실제 CPU에서 실행한 총 시간

    // 페이징 관련
    u_pt_t* page_table;
} pcb_t;

// PCB 큐 구조체
typedef struct {
    pcb_t* front;
    pcb_t* rear;
    int size;
} pcb_queue;

extern pcb_t* global_pcb_ptrs[10];

// Queue operations
pcb_queue* create_pcb_queue();
void enqueue_pcb(pcb_queue* queue, pcb_t* process);
pcb_t* dequeue_pcb(pcb_queue* queue, char* callback_name);
void remove_pcb(pcb_queue* queue, pid_t pid);
pcb_t* find_pcb(pcb_queue* queue, pid_t pid);
void print_pcb(pcb_t* pcb);
void print_ready_pcb_queue(pcb_queue* queue);
void print_waiting_pcb_queue(pcb_queue* queue);
void destroy_pcb_queue(pcb_queue* queue);
int is_queue_empty(pcb_queue* queue);
int get_queue_size(pcb_queue* queue);


// PCB 생성 및 관리 함수
pcb_t* create_pcb(pid_t pid, int cpu_burst, int priority);
void destroy_pcb(pcb_t* pcb);

#endif