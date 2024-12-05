#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include "msg2.h"
#include "vmm2.h"

#define TIME_TICK 100000
#define RUN_TIME 10000// 600초 == 1분 // 제출시에는 요구사항대로 10,000틱으로 설정할 예정
#define MSG_KEY 0x12345 * 3

// Global variables
FILE *file;
int run_time = 0;
int time_quantum;
int count = 0;
Queue runq;
Queue waitq;
Process* cur_proc;
int msg_queue;
PageDirectory *process_page_directories[MAX_CPROC];
ReplacementAlgorithm current_algorithm;

// Function declarations
void signal_handler(int signo);
void waitq_burst(void);
void init_process_memory(int pid);
void handle_memory_request(struct mem_msgbuf *mem_msg);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <time_quantum> <algorithm>\n", argv[0]);
        printf("Algorithms:\n");
        printf("0 - FIFO (First In First Out)\n");
        printf("1 - LRU (Least Recently Used)\n");
        return 1;
    }

    // Initialize
    time_quantum = atoi(argv[1]);
    int algo_choice = atoi(argv[2]);
    
    if (algo_choice != 0 && algo_choice != 1) {
        printf("Invalid algorithm choice. Using default (FIFO)\n");
        current_algorithm = ALGORITHM_FIFO;
    } else {
        current_algorithm = (ReplacementAlgorithm)algo_choice;
    }

    printf("Time Quantum: %d\n", time_quantum);
    printf("Selected Algorithm: %s\n", 
           current_algorithm == ALGORITHM_FIFO ? "FIFO" : "LRU");
    
    count = time_quantum;

    init_paging_system();

    // Timer setup
    struct itimerval new_itimer, old_itimer;
    new_itimer.it_interval.tv_sec = 0;
    new_itimer.it_interval.tv_usec = TIME_TICK;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;

    // Signal handler setup
    struct sigaction new_sa, old_sa;
    memset(&new_sa, 0, sizeof(new_sa));
    new_sa.sa_handler = &signal_handler;
    sigaction(SIGALRM, &new_sa, &old_sa);

    // Queue initialization
    initQueue(&runq);
    initQueue(&waitq);

    // Remove old message queue if exists
    int old_msgq = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(old_msgq != -1) {
        msgctl(old_msgq, IPC_RMID, NULL);
        printf("[System] Removed old message queue\n");
    }

    // Create new message queue
    msg_queue = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msg_queue == -1) {
        perror("msgget failed");
        return 1;
    }
    printf("[System] Created new message queue: %d\n", msg_queue);

    // Generate random bursts
    srand(time(NULL));
    int cpu_burst[MAX_CPROC];
    int io_burst[MAX_CPROC];
    for(int i = 0; i < MAX_CPROC; i++) {
        cpu_burst[i] = (rand() % 50) + 1;
        io_burst[i] = (rand() % 200) + 1;
    }

    // Create child processes
    for(int i = 0; i < MAX_CPROC; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } 
        // else if (pid == 0) {  // Child process
        //     struct msgbuf init_msg;
        //     init_msg.mtype = i + 1;
        //     init_msg.idx = i;
        //     init_msg.pid = getpid();
        //     init_msg.cpu_burst = cpu_burst[i];
        //     init_msg.io_burst = io_burst[i];
            
        //     msgsnd(msg_queue, &init_msg, sizeof(init_msg) - sizeof(long), 0);
        //     printf("[Child] Process %d (PID: %d) created\n", i, getpid());

        //     uint32_t proc_base = i * (1 << 20);
            
        //     // 자주 접근할 고정 주소 3개 설정
        //     uint32_t frequent_addresses[3];
        //     for(int k = 0; k < 3; k++) {
        //         frequent_addresses[k] = proc_base + (rand() % (1 << 20));
        //     }

        //     while(1) {
        //         kill(getpid(), SIGSTOP);
                
        //         struct mem_msgbuf mem_msg;
        //         mem_msg.mtype = i + 1;
        //         mem_msg.pid = getpid();
        //         mem_msg.process_index = i;
                
        //         for(int j = 0; j < 10; j++) {
        //             if (rand() % 10 < 1) {  // 10% 확률로 고정 주소 사용  // 약한 지역성 메모리 접근 패턴
        //                 mem_msg.virtual_addresses[j] = frequent_addresses[rand() % 3];
        //             } else {
        //                 uint32_t new_addr = proc_base + (rand() % (1 << 20));
        //                 mem_msg.virtual_addresses[j] = new_addr;
        //             }
        //         }
                    
        //         msgsnd(msg_queue, &mem_msg, sizeof(mem_msg) - sizeof(long), 0);
        //     }
        // }
        else if (pid == 0) {  // Child process
            struct msgbuf init_msg;
            init_msg.mtype = i + 1;
            init_msg.idx = i;
            init_msg.pid = getpid();
            init_msg.cpu_burst = cpu_burst[i];
            init_msg.io_burst = io_burst[i];
            
            msgsnd(msg_queue, &init_msg, sizeof(init_msg) - sizeof(long), 0);
            printf("[Child] Process %d (PID: %d) created\n", i, getpid());

            uint32_t proc_base = i * (1 << 20);
            uint32_t last_addr = proc_base;  // 마지막 접근 주소 저장
    
            // 자주 접근할 고정 주소 5개로 증가
            uint32_t frequent_addresses[5];
            for(int k = 0; k < 5; k++) {
                frequent_addresses[k] = proc_base + (rand() % (1 << 20));
            }

            while(1) {
                kill(getpid(), SIGSTOP);
                
                struct mem_msgbuf mem_msg;
                mem_msg.mtype = i + 1;
                mem_msg.pid = getpid();
                mem_msg.process_index = i;
                
                for(int j = 0; j < 10; j++) {
                    int access_type = rand() % 100;  // 접근 유형 결정
                    
                    if (access_type < 40) {  // 40% 확률로 최근 접근 영역 근처 // 시간적 지역성 강화
                        // 마지막 접근 주소 근처의 페이지 선택
                        int offset = (rand() % 8192) - 4096;  // ±4KB 범위
                        uint32_t new_addr = last_addr + offset;
                        if (new_addr < proc_base || new_addr >= proc_base + (1 << 20)) {
                            new_addr = proc_base + (rand() % (1 << 20));
                        }
                        mem_msg.virtual_addresses[j] = new_addr;
                        last_addr = new_addr;
                    }
                    else if (access_type < 70) {  // 30% 확률로 자주 사용하는 주소 //// 공간적 지역성 확보
                        mem_msg.virtual_addresses[j] = frequent_addresses[rand() % 5];
                    }
                    else {  // 30% 확률로 완전히 새로운 주소 // 아예 랜덤한 주소 접근 확률 대폭 낮춤
                        uint32_t new_addr = proc_base + (rand() % (1 << 20));
                        mem_msg.virtual_addresses[j] = new_addr;
                        last_addr = new_addr;
                    }
                }
                    
                msgsnd(msg_queue, &mem_msg, sizeof(mem_msg) - sizeof(long), 0);
            }
        }
    

        else {  // Parent process
            struct msgbuf rcv_msg;
            msgrcv(msg_queue, &rcv_msg, sizeof(rcv_msg) - sizeof(long), i+1, 0);

            init_process_memory(i);
            Process* new_proc = malloc(sizeof(Process));
            new_proc->pid = rcv_msg.pid;
            new_proc->idx = rcv_msg.idx;
            new_proc->cpu_burst = rcv_msg.cpu_burst;
            new_proc->io_burst = rcv_msg.io_burst;
            new_proc->remain_cpu_burst = rcv_msg.cpu_burst;
            new_proc->remain_io_burst = rcv_msg.io_burst;

            enqueue(&runq, new_proc);
            printf("[Parent] Process %d initialized and added to run queue\n", i);
        }
    }

    // Start first process
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu32.current_pd = process_page_directories[cur_proc->idx];
        kill(cur_proc->pid, SIGCONT);
    }

    // Start timer and main loop
    setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
    while(1) {
        pause();
    }

    return 0;
}

void signal_handler(int signo) {
    run_time++;

    if (cur_proc) {
        kill(cur_proc->pid, SIGSTOP);
    }

    print_qstate(run_time, &runq, &waitq);
    printf("[Timer] Time quantum: %d, Count: %d\n", time_quantum, count);

    waitq_burst();

    // Handle memory requests
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu32.current_pd = process_page_directories[cur_proc->idx];

        struct mem_msgbuf mem_msg;
        if (msgrcv(msg_queue, &mem_msg, sizeof(mem_msg) - sizeof(long), 
                   cur_proc->idx + 1, IPC_NOWAIT) != -1) {
            handle_memory_request(&mem_msg);
        }

        cur_proc->remain_cpu_burst--;
        count--;
    }

    // Process scheduling
    if (count == 0 || (cur_proc && cur_proc->remain_cpu_burst == 0)) {
        if (!isEmpty(&runq)) {
            Process* proc = dequeue(&runq);
            
            if (proc->remain_cpu_burst > 0) {
                enqueue(&runq, proc);
            } else {
                enqueue(&waitq, proc);
            }
            count = time_quantum;
        }
    }

    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu32.current_pd = process_page_directories[cur_proc->idx];
        kill(cur_proc->pid, SIGCONT);
    }

    if (run_time % 10 == 0) {
        print_memory_status();
    }

    if (run_time == RUN_TIME) {
        printf("\n[System] Simulation finished\n");
        //print_memory_status();
        print_vm_stats();              // 통계 출력 추가
        
        // 메시지 큐 정리
        msgctl(msg_queue, IPC_RMID, NULL);
        
        // 자식 프로세스들 정리 - 수정된 부분
        for (int i = 0; i < runq.size; i++) {
            Process* proc = getProcessAtIndex(&runq, i);
            if (proc) {
                kill(proc->pid, SIGTERM);
            }
        }
        
        for (int i = 0; i < waitq.size; i++) {
            Process* proc = getProcessAtIndex(&waitq, i);
            if (proc) {
                kill(proc->pid, SIGTERM);
            }
        }
    
        // 동적 할당된 메모리 해제
        for (int i = 0; i < MAX_CPROC; i++) {
            if (process_page_directories[i]) {
                for (int j = 0; j < PAGE_DIR_SIZE; j++) {
                    if (process_page_directories[i]->tables[j]) {
                        free(process_page_directories[i]->tables[j]);
                    }
                }
                free(process_page_directories[i]);
            }
        }
        
        if (swap_space) {
            free(swap_space);
        }
        
        printf("[System] Cleanup completed\n");
        exit(0);
    }
}

void waitq_burst() {
    int initial_size = waitq.size;
    
    while (initial_size > 0) {
        Process* proc = dequeue(&waitq);
        initial_size--;
        
        proc->remain_io_burst--;
        
        if (proc->remain_io_burst == 0) {
            proc->remain_cpu_burst = proc->cpu_burst;
            proc->remain_io_burst = proc->io_burst;
            enqueue(&runq, proc);
            printf("[I/O] Process %d completed I/O\n", proc->idx);
        } else {
            enqueue(&waitq, proc);
        }
    }
}