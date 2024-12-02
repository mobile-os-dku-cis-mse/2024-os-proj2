//
// Created by hochacha on 24. 11. 6.
//

#include "child.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>

#include "src/util/message_queue.h"

//#define DEBUG
int cpu_burst = 0;  // 한 프로그램이 실행되는데 필요한 CPU 시간
int io_burst = 0;   // 한 프로그램이 요청하는 IO 작업의 시간
int cpu_time = 0;   // 스케줄로 부여 받는 시간
int io_time = 0;    // IO로 요청하는 시간
int total_exec_time = 0;
// 따라서, CPU Time 전부 소진하면, IO Time 만큼 IO 수행 요청

int process_state = 0;
int EOP = 0; // End of Process

volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigusr2_received = 0;

extern FILE* mem_access;

void request_io_msgsnd(int requesting_io_time, int new_cpu_burst, int new_io_burst) {
    io_msg msg;
    memset(&msg, 0, sizeof(io_msg));
    msg.msg_t = 0;
    msg.pid = getpid();
    msg.io_time = requesting_io_time;
    msg.new_cpu_burst = new_cpu_burst;
    msg.new_io_burst = new_io_burst;
    msg.is_finished = EOP;
    if(msgsnd(msg_queue_id_sched, &msg, sizeof(io_msg), IPC_NOWAIT) == -1) {
        perror("child_msgsnd");
        exit(EXIT_FAILURE);
    }
}

// SIGALRM 핸들러: 타이머 틱마다 호출되어 CPU 버스트를 감소시킴
void child_SIGALRM(int sig) {
    if (process_state == PROCESS_RUNNING && !EOP) {
        cpu_time--;
        cpu_burst--;
        total_exec_time++;
#ifdef DEBUG
        printf("[Child::%d] CPU burst remaining: %d\n", getpid(), cpu_burst);
#endif
    }
#ifdef DEBUG
    printf("[Child::%d] cpu_burst = %d, state = %d\n", getpid(), cpu_burst, process_state);
#endif
    // CPU 버스트가 종료되면 IO 버스트 요청
    if (cpu_burst <= 0 && process_state == PROCESS_RUNNING) {
#ifdef DEBUG
        printf("[Child::%d] CPU burst completed. Requesting IO.\n", getpid());
#endif
        process_state = PROCESS_BLOCKED;
        int new_cpu_burst = rand() % 20 + 1;
        int new_io_burst = rand() % 20 + 1;

        request_io_msgsnd(io_burst, new_cpu_burst, new_io_burst);
        total_exec_time += io_burst;
        cpu_burst = new_cpu_burst;
        io_burst = new_io_burst;
        cpu_time = TIME_QUANTUAM;
    }
}

// 스케줄 알림
void child_SIGUSR1(int sig) {
#ifdef PROJ1
    printf("[Child::%d] SIGUSR1 cpu_burst = %d, state = %d\n", getpid(), cpu_burst, process_state);
#endif
    cpu_time = TIME_QUANTUAM;
    process_state = PROCESS_RUNNING;
}

// 스케줄 아웃 처리
void child_SIGUSR2(int sig) {
#ifdef PROJ1
    printf("[Child::%d] SIGUSR2 cpu_burst = %d, state = %d\n", getpid(), cpu_burst, process_state);
#endif
    process_state = PROCESS_READY;
}

void child_SIGINT(int sig) {
    EOP = 1;
}

void init_child() {
    // enroll the SIGNAL Handler

    if(signal(SIGUSR1, child_SIGUSR1) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if(signal(SIGUSR2, child_SIGUSR2) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if(signal(SIGINT, child_SIGINT) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    srand(time(NULL));
    cpu_burst = rand() % 10 + 1;
    io_burst = rand() % 10 + 1;

#ifdef PROJ1
    printf("[Child::%d] cpu_burst = %d, io_burst = %d\n", getpid(),cpu_burst, io_burst);
#endif

    // get idential queues for each child
    if(init_msg_queue() == -1) {
        perror("init_msg_queue");
        exit(1);
    }
#ifdef PROJ1
    printf("[do_work_child] init child process\n");
#endif
    if(signal(SIGALRM, child_SIGALRM) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
}

void vaddr_access(int vaddr) {
    vaddr_t msg ={0,};
    msg.msg_t = MESSAGE_TYPE_VADDR_ACCESS;
    msg.pid = getpid();
    msg.access_mem = vaddr;

    if(msgsnd(msg_queue_id_page, &msg, sizeof(vaddr_t), IPC_NOWAIT) == -1) {
        perror("child_msgsnd");
        exit(EXIT_FAILURE);
    }

    /*

     */
    if (msgget(msg_queue_id_page, &msg)) {

    }
}

int child_main(int argc, char **argv) {
    init_child();
#ifdef DEBUG
    printf("[Child::%d] ready to run\n", getpid());
#endif
    while(!EOP) {
        if (process_state == PROCESS_RUNNING) {

            fprintf(mem_access, "print out: %d\n", getpid());
            fflush(mem_access);     // printf 시에는 반드시 fflush로 버퍼 비워줄 것

            for (int i = 0; i < 10; i++) {

            }
        }
        usleep(100);
    }
}