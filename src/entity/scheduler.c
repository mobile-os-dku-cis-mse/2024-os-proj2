//
// Created by hochacha on 24. 11. 6.
//

#include "scheduler.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include "../util/timer/timer.h"
#include "../util/pid_queue/pid_queue.h"
#include "../util/msg_queue/message_queue.h"
#include <pthread.h>
#include "page_manager/page_manager.h"
#include "src/util/frame_list/frame_list.h"
#include "src/util/swap/swap.h"

//#define DEBUG
//#define SIGALRM_DEBUG
//#define PROJ1

#define METRICS
unsigned int current_time = 0;
pcb_queue* ready_queue_schd = NULL;
pcb_queue* waiting_queue_schd = NULL;
pcb_t* current_process = NULL;
u_pt_t* pt_base_register = NULL;
pthread_mutex_t ptbr_mutex = PTHREAD_MUTEX_INITIALIZER;



volatile sig_atomic_t io_request_received = 0;
extern FILE* IO_result;

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        pcb_t* process;
        while ((process = dequeue_pcb(ready_queue_schd, "sigint_handler")) != NULL) {
            process->waiting_time += current_time - process->arrival_time;
            kill(process->pid, SIGTERM);
            waitpid(process->pid, NULL, 0);
	    process->completion_time = process->response_time + process->waiting_time + process->execution_time;
#ifdef PROJ1
            printf("Process %d Metrics:\n", process->pid);
            printf("  Waiting Time: %u\n", process->waiting_time);
            printf("  Response Time: %u\n", process->response_time);
            printf("  Execution Time: %u\n", process->execution_time);
            printf("  Completion Time: %lu\n", process->completion_time);
#endif
            destroy_pcb(process);
        }
        // Waiting 큐에 있는 모든 프로세스 종료
        while ((process = dequeue_pcb(waiting_queue_schd, "sigint_handler")) != NULL) {
            kill(process->pid, SIGTERM);
            waitpid(process->pid, NULL, 0);
            destroy_pcb(process);
        }
        // 현재 실행 중인 프로세스 종료
        if (current_process != NULL) {
            // 실행 시간 업데이트
            current_process->execution_time += TIME_QUANTUAM - current_process->remaining_time;
            current_process->completion_time = current_process->response_time + current_process->waiting_time + current_process->execution_time;
            kill(current_process->pid, SIGTERM);
            waitpid(current_process->pid, NULL, 0);
#ifdef PROJ1
            printf("Process %d Metrics:\n", current_process->pid);
            printf("  Waiting Time: %u\n", current_process->waiting_time);
            printf("  Response Time: %u\n", current_process->response_time);
            printf("  Execution Time: %u\n", current_process->execution_time);
            printf("  Completion Time: %lu\n", current_process->completion_time);
#endif
            destroy_pcb(current_process);
        }
        msgctl(msg_queue_id_sched, IPC_RMID, NULL);  // 메시지 큐 삭제
        exit(0);
    }
}
void handle_io_from_child_by_checking_ipc() {
    io_msg msg;
    memset(&msg, 0, sizeof(msg));
    if((msgrcv(msg_queue_id_sched, &msg, sizeof(io_msg) - sizeof(int), 0, IPC_NOWAIT)) != -1) {
        pid_t pid = msg.pid;
        unsigned int io_time = msg.io_time;
        int is_finished = msg.is_finished;
        int new_cpu_time = msg.new_cpu_burst;
        int new_io_time = msg.new_io_burst;
#ifdef DEBUG
        printf("[IO Handler] received the msg by child %d, io time = %d, is_finished = %d, new_cpu_time = %d, new_io_t ime = %d\n",
            msg.pid, msg.io_time, msg.new_cpu_burst, msg.new_io_burst);
#endif
        if(current_process != NULL && current_process->pid == pid) {
            current_process->state = PROCESS_BLOCKED;
            current_process->io_time = io_time;
            if(is_finished) {
                current_process->state = PROCESS_TERMINATED;

            }
            if(io_time) {
                // 현재 프로세스가 IO 요청을 할 것임
                kill(current_process->pid, SIGUSR2);
                current_process->io_burst_time = new_io_time;
                current_process->cpu_burst_time = new_cpu_time;
                enqueue_pcb(waiting_queue_schd, current_process);
#ifdef DEBUG
                printf("[IO Handler] Current process = %d has been enqueued into waiting queue\n", current_process->pid);
#endif

                //current_process = dequeue_pcb(ready_queue_schd, "handle_io_from_child_by_checking_ipc");
                //kill(current_process->pid, SIGUSR1);
                //printf("[IO Handler] Current process = %d has been dequeued from ready queue\n", current_process->pid);
            }
        }
    }
}

void decrease_IO_time() {
    // 대기 큐를 먼저 처리
    int waiting_queue_size = get_queue_size(waiting_queue_schd);
    for(int i = 0; i < waiting_queue_size; i++) {
        pcb_t * process = dequeue_pcb(waiting_queue_schd, "decrease_IO_time");
        process->io_time--;

        if(process->io_time <= 0) {
            process->state = PROCESS_READY;
#ifdef PROJ1
            print_pcb(current_process);
#endif
            // metric 초기화
            process->arrival_time = current_time;
            process->start_time = 0;
            process->waiting_time = 0;
            process->response_time = 0;
            process->execution_time = 0;
            enqueue_pcb(ready_queue_schd, process);
            kill(process->pid, SIGUSR2);  // 자식 프로세스에 IO 완료 알림
        } else {
            enqueue_pcb(waiting_queue_schd, process);
        }
    }
}

void alarm_handler(int sig) {
#ifdef TICK
    printf("[Tick:: %d]\n", current_time);
#endif
    if(current_process != NULL) {
#ifdef PROJ1
        printf("[Scheduler] Current Process = [%d(%d:%d)]\n", current_process->pid, current_process->cpu_burst_time, current_process->io_burst_time);
#endif
    }
    else {
#ifdef PROJ1
        printf("[Scheduler] No current process\n");
#endif
    }
#ifdef PROJ1
    print_ready_pcb_queue(ready_queue_schd);
    print_waiting_pcb_queue(waiting_queue_schd);
#endif
    current_time++;  // 전역 시간 증가
    decrease_IO_time();
    handle_io_from_child_by_checking_ipc();

    // 대기 시간 증가 구문
    pcb_t* temp = ready_queue_schd->front;
    while (temp != NULL) {
        temp->waiting_time++;
        temp = temp->next;
    }

#ifdef SIGALRM_DEBUG
    printf("1\n");
#endif
    // 현재 실행 중인 프로세스의 남은 시간 감소
    if(current_process != NULL && current_process->state == PROCESS_RUNNING
                                        && current_process->remaining_time > 0) {
        kill(current_process->pid, SIGALRM);
        current_process->remaining_time--;
        current_process->cpu_burst_time--;
        current_process->execution_time++;
#ifdef SIGALRM_DEBUG
        printf("2\n");
#endif
        // 타임 퀀텀 소진 시 스케줄 아웃
#ifdef DEBUG
        printf("[Tick :: %d] Process %d: remain time = %d\n",
            current_time, current_process->pid, current_process->remaining_time);
#endif
        if(current_process->remaining_time <= 0) {
#ifdef PROJ1
            printf("[Scheduler::%d] Schedule out ", getpid());
            printf("[Current process = %d]\n",current_process->pid);
#endif
            kill(current_process->pid, SIGUSR2);
            current_process->state = PROCESS_READY;
            enqueue_pcb(ready_queue_schd, current_process);

            current_process = dequeue_pcb(ready_queue_schd, "Time out schedule out");
            pthread_mutex_lock(&ptbr_mutex);
            pt_base_register = current_process->page_table;
            pthread_mutex_unlock(&ptbr_mutex);
#ifdef METRICS
            if (current_process->start_time == 0) {
                current_process->start_time = current_time;
                current_process->response_time = current_time - current_process->arrival_time;
            }
#endif
            current_process->state = PROCESS_RUNNING;
            current_process->remaining_time = TIME_QUANTUAM;
            kill(current_process->pid, SIGUSR1);
#ifdef PROJ1
            printf("========================================\n");
            printf("[Scheduler] Schedule In %d\n", current_process->pid);
            printf("========================================\n");
#endif
        }
    }else {
        // 현재 실행 중인 프로세스가 없는 경우
#ifdef SIGALRM_DEBUG
        printf("5\n");
#endif
        if(!is_queue_empty(ready_queue_schd)) {
#ifdef SIGALRM_DEBUG
            printf("6\n");
#endif
            current_process = dequeue_pcb(ready_queue_schd, "No current process");
            pthread_mutex_lock(&ptbr_mutex);
            pt_base_register = current_process->page_table;
            pthread_mutex_unlock(&ptbr_mutex);
            current_process->state = PROCESS_RUNNING;
            current_process->remaining_time = TIME_QUANTUAM;
            kill(current_process->pid, SIGUSR1);

            if (current_process->start_time == 0) {
                current_process->start_time = current_time;
                current_process->response_time = current_time - current_process->arrival_time;
            }
#ifdef PROJ1
            printf("========================================\n");
            printf("[Scheduler] Schedule In %d\n", current_process->pid);
            printf("========================================\n");
#endif

            current_time--;
        } else {
            // 디큐할 거 없으면 틱 그냥 넘기기
#ifdef SIGALRM_DEBUG
            printf("7\n");
#endif
            if(!is_queue_empty(waiting_queue_schd)) {
                decrease_IO_time();
#ifdef SIGALRM_DEBUG
                printf("8\n");
#endif
            }else {
                // 프로세스가 아무것도 없는 경우
#ifdef PROJ1
                printf("[Scheduler] End of Scheduling\n");
#endif
                kill(getpid(), SIGINT);
                exit(0);
            }
        }
    }
}

void scheduler_init(pcb_queue* ready_queue) {
#ifdef DEBUG
    printf("[Tick :: %d] child process starts being scheduled\n", current_time);
#endif

    init_physical_memory();
    init_swap_space();


    if(signal(SIGUSR1, sigint_handler) == SIG_ERR) {
        perror("signal");
        sigint_handler(SIGINT);
        exit(1);
    }

    if(signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        sigint_handler(SIGINT);
        exit(1);
    }

    ready_queue_schd = ready_queue;
    waiting_queue_schd = create_pcb_queue();

    setup_timer();

    initialize_page_manager();
#ifdef DEBUG
    printf("[Tick :: %d] scheduler lunch\n", current_time);
#endif
}

void scheduler_run(pcb_queue* ready_queue, int n_process) {
    scheduler_init(ready_queue);

    // just work with handler function
    while(1) {
        if(current_time > 10000) {
            break;
        }
    };
    sigint_handler(SIGINT);
    printf("End\n");
}

