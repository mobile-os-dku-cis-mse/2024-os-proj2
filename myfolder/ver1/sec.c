// sec.c
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
#include "msg.h"
#include "vmm.h"

#define MAX_CPROC 10
#define TIME_TICK 100000  // 0.1초
#define RUN_TIME 10000//6000    // 총 실행 시간 (제출 시 10000으로 바꾸어 제출할 것)
#define MSG_KEY 0x12345 * 2

// 전역 변수
FILE *file;
int run_time = 0;
int time_quantum;
int count = 0;
Queue runq;
Queue waitq;
Process* cur_proc;
int msg_queue;

// 함수 선언
void signal_handler(int signo);
void waitq_burst(void);

int main(int argc, char* argv[]) {
    // 인자 확인
    if (argc < 2) {
        printf("Usage: %s <time_quantum>\n", argv[0]);
        return 1;
    }

    // 타임 퀀텀 설정
    time_quantum = atoi(argv[1]);
    printf("Time Quantum: %d\n", time_quantum);
    count = time_quantum;

    // 로그 파일 생성 : 이전 과제에서 사용
    // file = fopen("schedule_dump.txt", "w");
    // if (file == NULL) {
    //     perror("Failed to open file");
    //     return 1;
    // }

    // 가상 메모리 시스템 초기화
    init_physical_memory();
    init_free_frame_list();
    init_mmu();

    // 타이머 설정
    struct itimerval new_itimer, old_itimer;
    new_itimer.it_interval.tv_sec = 0;
    new_itimer.it_interval.tv_usec = TIME_TICK;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;

    // 시그널 핸들러 등록
    struct sigaction new_sa, old_sa;
    memset(&new_sa, 0, sizeof(new_sa));
    new_sa.sa_handler = &signal_handler;
    sigaction(SIGALRM, &new_sa, &old_sa);

    // 큐 초기화
    initQueue(&runq);
    initQueue(&waitq);

     int old_msgq = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(old_msgq != -1) {
        if(msgctl(old_msgq, IPC_RMID, NULL) == -1) {
            perror("Failed to remove old message queue");
            return 1;
        }
        printf("[System] Removed old message queue\n");
    }

    // 새로운 메시지 큐 생성
    msg_queue = msgget(MSG_KEY, IPC_CREAT | 0666);
    if(msg_queue == -1) {
        perror("msgget failed");
        return 1;
    }
    printf("[System] Created new message queue: %d\n", msg_queue);

// CPU burst와 I/O burst 랜덤 생성
    srand(time(NULL));
    int cpu_burst[MAX_CPROC];
    int io_burst[MAX_CPROC];
    for(int j = 0; j < MAX_CPROC; j++) {
        cpu_burst[j] = (rand() % 50) + 1;
        io_burst[j] = (rand() % 200) + 1;
    }

    // 자식 프로세스 생성
    for(int i = 0; i < MAX_CPROC; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } 
        else if (pid == 0) {  // 자식 프로세스
            // 초기 프로세스 정보 전송
            struct msgbuf init_msg;
            init_msg.mtype = i + 1;
            init_msg.idx = i;
            init_msg.pid = getpid();
            init_msg.cpu_burst = cpu_burst[i];
            init_msg.io_burst = io_burst[i];
            
            if (msgsnd(msg_queue, &init_msg, sizeof(init_msg) - sizeof(long), 0) == -1) {
                perror("msgsnd failed");
                exit(1);
            }

            printf("[Child] Process %d (PID: %d) created\n", i, getpid());

            // 실행 중 메모리 접근 요청
            while(1) {
                kill(getpid(), SIGSTOP);
                
                // 새로운 10개의 가상 주소 생성 및 전송
                struct mem_msgbuf mem_msg;
                mem_msg.mtype = i + 1;
                mem_msg.pid = getpid();
                mem_msg.process_index = i;
                
                for(int j = 0; j < 10; j++) {
                    mem_msg.virtual_addresses[j] = rand() & 0xFFFF;
                }
                
                if (msgsnd(msg_queue, &mem_msg, sizeof(mem_msg) - sizeof(long), 0) == -1) {
                    perror("Memory access request failed");
                }
            }
            exit(0);
        }
else {  // 부모 프로세스
            // 자식 프로세스의 초기 정보 수신
            struct msgbuf rcv_msg;
            if (msgrcv(msg_queue, &rcv_msg, sizeof(rcv_msg) - sizeof(long), i+1, 0) == -1) {
                perror("msgrcv failed");
                exit(1);
            }

            printf("[Parent] Received initial info from Process %d\n", i);
            printf("├── PID: %d\n", rcv_msg.pid);
            printf("├── CPU Burst: %d\n", rcv_msg.cpu_burst);
            printf("└── I/O Burst: %d\n", rcv_msg.io_burst);

            // 프로세스 객체 생성 및 초기화
            Process* new_proc = malloc(sizeof(Process));
            new_proc->pid = rcv_msg.pid;
            new_proc->idx = rcv_msg.idx;
            new_proc->io_burst = rcv_msg.io_burst;
            new_proc->remain_io_burst = rcv_msg.io_burst;
            new_proc->cpu_burst = rcv_msg.cpu_burst;
            new_proc->remain_cpu_burst = rcv_msg.cpu_burst;

            // 페이지 테이블 초기화
            for(int j = 0; j < PAGE_TABLE_SIZE; j++) {
                page_tables[i].entries[j].valid_bit = 0;
                page_tables[i].entries[j].dirty_bit = 0;
                page_tables[i].entries[j].referenced_bit = 0;
                page_tables[i].entries[j].protection_bits = 7;  // RWX 권한
            }

            // 실행 큐에 프로세스 추가
            if (!isFull(&runq)) {
                enqueue(&runq, new_proc);
                printf("[Parent] Process %d added to run queue\n", i);
            } else {
                printf("[Parent] Run queue is full\n");
                free(new_proc);
            }
        }
    }

    // 초기 상태 출력
    printf("\n[Initial State] All processes created\n");
    print_qstate(run_time, &runq, &waitq);
    print_memory_status();

    // 첫 번째 프로세스 실행
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu.ptbr = cur_proc->idx;  // PTBR 설정
        kill(cur_proc->pid, SIGCONT);
    }

    // 타이머 시작
    setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

    // 메인 루프
    while(1) {
        pause();
    }

    return 0;
}

// 시그널 핸들러
void signal_handler(int signo) {
    run_time++;
    current_time = run_time;  // 전역 시간 업데이트
    count--;

    // 현재 실행 중인 프로세스가 있으면 중지
    if (cur_proc != NULL) {
        kill(cur_proc->pid, SIGSTOP);
    }

    print_qstate(run_time, &runq, &waitq);
    //writeToFile(file, run_time, count, &runq, &waitq);
    printf("[Timer] Time quantum: %d, Count: %d\n", time_quantum, count);

    // Wait Queue 처리
    waitq_burst();

    // 현재 실행 중인 프로세스의 메모리 접근 처리
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu.ptbr = cur_proc->idx;  // PTBR 업데이트

        // 메모리 접근 요청 수신
        struct mem_msgbuf mem_msg;
        if (msgrcv(msg_queue, &mem_msg, sizeof(mem_msg) - sizeof(long), 
                   cur_proc->idx + 1, IPC_NOWAIT) != -1) {
            printf("\n[Memory] Processing memory access requests for Process %d\n", cur_proc->idx);
            // 10개의 메모리 접근 처리
            for(int i = 0; i < 10; i++) {
                handle_memory_access(mem_msg.virtual_addresses[i]);
            }
        }

        // CPU 버스트 처리
        cur_proc->remain_cpu_burst--;
    }

    // 타임 퀀텀 만료 또는 CPU 버스트 완료 체크
    if (count == 0 || (cur_proc && cur_proc->remain_cpu_burst == 0)) {
        if (!isEmpty(&runq)) {
            Process* proc = dequeue(&runq);
            
            if (proc->remain_cpu_burst > 0) {
                // 아직 CPU 버스트가 남았으면 다시 실행 큐로
                enqueue(&runq, proc);
            } else {
                // CPU 버스트 완료되면 Wait 큐로
                enqueue(&waitq, proc);
            }
            count = time_quantum;  // 타임 퀀텀 리셋
        }
    }

    // 다음 프로세스 실행
    if (!isEmpty(&runq)) {
        cur_proc = &runq.processes[runq.front];
        mmu.ptbr = cur_proc->idx;  // PTBR 업데이트
        kill(cur_proc->pid, SIGCONT);
    }

    // 주기적으로 메모리 상태 출력
    if (run_time % 10 == 0) {
        print_memory_status();
    }

     // 종료 조건 체크
    if (run_time == RUN_TIME) {
        printf("\n[System] Simulation finished after %d ticks\n", run_time);
        print_performance_metrics();  // 성능 지표 출력
        
        // 메시지 큐 제거
        if(msgctl(msg_queue, IPC_RMID, NULL) == -1) {
            perror("Failed to remove message queue");
        } else {
            printf("[System] Removed message queue\n");
        }
        
        exit(0);
    }
}

// Wait Queue의 I/O 버스트 처리
void waitq_burst() {
    int initial_size = waitq.size;
    
    while (initial_size > 0) {
        Process* proc = dequeue(&waitq);
        initial_size--;
        
        proc->remain_io_burst--;
        
        if (proc->remain_io_burst == 0) {
            // I/O 버스트 완료되면 CPU 버스트 초기화하고 실행 큐로
            proc->remain_cpu_burst = proc->cpu_burst;
            proc->remain_io_burst = proc->io_burst;
            enqueue(&runq, proc);
            printf("[I/O] Process %d completed I/O, moved to run queue\n", proc->idx);
        } else {
            // 아직 I/O 버스트가 남았으면 다시 Wait 큐로
            enqueue(&waitq, proc);
        }
    }
}