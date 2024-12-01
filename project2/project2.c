#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

#define CHILDNUM 10
#define PDIRNUM 1024
#define PTLBNUM 1024
#define FRAMENUM 1024 // 4B X 1KB = 4KB
#define QNUM 32
#define Time_Q 4

struct Proc_t{ // PCB
    pid_t pid;
    int burst;
    int time_quantum;
    struct Proc_t *next; // Next Node
};

struct Queue_t{ // Run Queue
    struct Proc_t* front;
    struct Proc_t* rear;
    int count;
};

typedef struct{ // 2nd table
    int valid;
    int pfn;
}TABLE;

typedef struct{ // 1st table
    int valid;
    TABLE* pt;
}PDIR;

struct msg_buffer{
    long mtype;
    int pid_index;
    unsigned int vm[10];
};

PDIR pdir[CHILDNUM][PDIRNUM];
pid_t pids[CHILDNUM]; // PID per Process

struct Queue_t runq_t; // Tick: 1s

struct msg_buffer msg;
int msgq;
int ret;
int key = 0x12345;
int count = 0;
int process_time[10]; // Burst time per process
int state = 0;

int random_burst_time = 0; // Burst time per process
int total_burst_time = 0;
int RQ[QNUM]; // RUN QUEUE
int head = 0, tail = 0;
int free_page_list[1024];
int fpl_head = 0, fpl_tail = 0; // FREE PAGE LIST head, tail
int idx = 0, total_tik = 0, proc_done = 0;

FILE* fd = NULL;

void init_queue(struct Queue_t* q);
void add_queue(struct Queue_t *rq, int pid, int burst, int tq);
struct Proc_t* pop_queue(struct Queue_t* rq);
void signal_handler(int signo);
void signal_handler2(int signo);
void toFreePageList(PDIR* p_dir);
int MMU(int free_page_list[], int index, int vm);

void init_queue(struct Queue_t* q){
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

void add_queue(struct Queue_t *rq, int pid, int burst, int tq){
    struct Proc_t *temp = malloc(sizeof(struct Proc_t));

    temp->pid = pid;
    temp->burst = burst;
    temp->time_quantum = tq;
    temp->next = NULL;

    if(rq->count == 0){ // Queue Empty or not
        rq->front = temp;
        rq->rear = temp;
    } else{
        rq->rear->next = temp;
        rq->rear = temp;
    }
    rq->count++;
}

struct Proc_t* pop_queue(struct Queue_t* rq){ // Return Process
    if (rq->count == 0) return NULL; // Handle empty queue

    struct Proc_t* temp = rq->front;
    rq->front = temp->next;
    rq->count--;

    return temp; // Return the removed node
}

int main(){
    fd = fopen("schedule_dump.txt", "w");

    if(fd == NULL){
        perror("Failed to Create txt file\n");
        exit(0);
    }

    unsigned int vm[10];
    unsigned int pdir_idx[10];
    unsigned int tbl_idx[10];
    unsigned int offset[10];
    int pid_index;
    int physical_addr = 0;

    init_queue(&runq_t);
    srand(time(NULL));
    msgq = msgget(key, IPC_CREAT | 0666);

    // Init Processes
    for(int i = 0; i < 10; i++){
        pids[i] = 0; // Initialize PID
        process_time[i] = rand() % 50 + 100; // Set Burst TIme
    }

    // Init FreePageLists
    for(int i = 0; i < FRAMENUM; i++){
        free_page_list[i] = i;
        fpl_tail++;
    }

    for(int i = 0; i < CHILDNUM; i++){
        RQ[(tail++) % QNUM] = idx;

        // Init PDIR
        for(int k = 0; k < CHILDNUM; k++){
            for(int j = 0; j < PDIRNUM; j++){
                pdir[k][j].valid = 0;
                pdir[k][j].pt = NULL;
            }
        }

        int ret = fork();

        if(ret < 0){
            perror("fork failed");
        } else if(ret == 0){ // Child
            // Signal handler setup
            struct sigaction old_sa;
            struct sigaction new_sa;
            memset(&new_sa, 0, sizeof(new_sa));
            new_sa.sa_handler = &signal_handler2;
            sigaction(SIGALRM, &new_sa, &old_sa);
            // Execution Time
            random_burst_time = process_time[i];

            // Repeating
            while(1){pause();}

        } else if(ret > 0){ // Parent
            total_burst_time += process_time[i];
            pids[i] = ret;
            printf("Child %d created, BT: %ds\n", pids[i], process_time[i]);

            // Put it in the RunQ
            add_queue(&runq_t, pids[i], process_time[i], Time_Q-1);
        }
        idx++;
    }

    // signal handler setup
    struct sigaction old_sa;
    struct sigaction new_sa;
    memset(&new_sa, 0, sizeof(new_sa));
    new_sa.sa_handler = &signal_handler;
    sigaction(SIGALRM, &new_sa, &old_sa);

    // fire the alarm timer
    struct itimerval new_itimer, old_itimer;
    new_itimer.it_interval.tv_sec = 1;
    new_itimer.it_interval.tv_usec = 0;
    new_itimer.it_value.tv_sec = 1;
    new_itimer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

    while(1){
        ret = msgrcv(msgq, &msg, sizeof(msg), 0, IPC_NOWAIT);

        if(ret != -1){
            pid_index = msg.pid_index;

            for(int k = 0; k < CHILDNUM; k++){
                vm[k] = msg.vm[k];

                pdir_idx[k] = (vm[k] & 0xffc00000) >> 22; // 10 bit
                tbl_idx[k] = (vm[k] & 0x3ff000) >> 12; // 10 bit
                offset[k] = vm[k] & 0xfff;

                // First Paging
                if(pdir[pid_index][pdir_idx[k]].valid == 0){
                    printf("1-Level PAGE FAULT\n");
                    fprintf(fd, "1-Level PAGE FAULT\n");
                    TABLE* table = (TABLE*)calloc(PTLBNUM, sizeof(TABLE));
                    pdir[pid_index][pdir_idx[k]].pt = table;
                    pdir[pid_index][pdir_idx[k]].valid = 1;
                }

                TABLE* ptbl = pdir[pid_index][pdir_idx[k]].pt;

                // Second Paging
                if(ptbl[tbl_idx[k]].valid == 0){
                    printf("2-Level PAGE FAULT\n");
                    fprintf(fd, "2-Level PAGE FAULT\n");

                    if(fpl_head != fpl_tail || (runq_t.count > 0)){
                        ptbl[tbl_idx[k]].pfn = free_page_list[(fpl_head % FRAMENUM)];
                        ptbl[tbl_idx[k]].valid = 1;
                        fpl_head++;
                    }
                    else{
                        for(int k = 0; k < CHILDNUM; k++){
                            kill(pids[k],SIGKILL);
                        }
                        msgctl(msgq, IPC_RMID, NULL);
                        exit(0);
                        return 0;
                    }
                }

                physical_addr = MMU(free_page_list, fpl_head, vm[k]);

                printf("PID: %d -> ", pids[k]);
                printf("VA: 0x%08x[PDIR: 0x%03x, PTBL: 0x%03x, OFFSET: 0x%04x] -> PA: 0x%08x\n", vm[k], pdir_idx[k], tbl_idx[k], offset[k], physical_addr);
                fprintf(fd, "PID: %d -> ", pids[k]);
                fprintf(fd, "VA: 0x%08x[PDIR: 0x%03x, PTBL: 0x%03x, OFFSET: 0x%04x] -> PA: 0x%08x\n", vm[k], pdir_idx[k], tbl_idx[k], offset[k], physical_addr);
            }
            memset(&msg, 0, sizeof(msg));
        }
    }

    while (1);
    fclose(fd);
    return 0;
}

int MMU(int free_page_list[], int index, int vm){
    int offset = vm & 0xfff;
    int physical_addr;
    int pfn;

    pfn = free_page_list[(fpl_head % FRAMENUM)];
    physical_addr = (pfn << 12) + offset;

    return physical_addr;
}

void signal_handler2(int signo){ // Signal Child(User) -> Parent(OS)
    count++;
    memset(&msg, 0, sizeof(msg));
    msg.mtype = IPC_NOWAIT;
    msg.pid_index = idx;

    unsigned int temp_add;
    for(int k = 0; k < 10; k++){ // Random Memory Access
        temp_add = (rand() % 0x4) << 22; // 10 bit for directory page table
        temp_add |= (rand() % 0x20) << 12; // 10 bit for page table
        temp_add |= (rand() % 0xfff); // 12 bit for offset
        msg.vm[k] = temp_add;
    }

    ret = msgsnd(msgq, &msg, sizeof(msg), IPC_NOWAIT);

    if(count == random_burst_time){
        printf("---Child(%d) CPU Execution Completed---\n\n", getpid());
        idx++;
        exit(0);
    }
}

void toFreePageList(PDIR* p_dir){
    for(int i = 0; i < PDIRNUM; i++){
        if(p_dir[i].valid == 1){
            p_dir[i].valid = 0;
            if(p_dir[i].pt != NULL){
                for(int j = 0; j < PTLBNUM; j++){
                    if(p_dir[i].pt[j].valid == 1){
                        free_page_list[(fpl_tail++) % FRAMENUM] = p_dir[i].pt[j].pfn;
                    }
                }
                free(p_dir[i].pt); // Free memory before nullifying
                p_dir[i].pt = NULL;
            }
        }
    }
}

void signal_handler(int signo){ // Signal Parent(OS) -> Child(User)
    struct Proc_t* p = malloc(sizeof(struct Proc_t));
    p = runq_t.front;
    int target_pid = p->pid;

    if(proc_done == 1){
        toFreePageList(pdir[RQ[(head-1)%QNUM]]);
        proc_done = 0;
    }

    count++;
    printf("\n----------------Scheduler----------------\n");
    printf("Count: %d\n", count);
    printf("Total burst time: %d\n", total_burst_time);

    fprintf(fd, "\n----------------Scheduler----------------\n");
    fprintf(fd, "Count: %d\n", count);
    fprintf(fd, "Total burst time: %d\n", total_burst_time);

    int burst = 0; // Remaining Burst Time

    // Run queue
    printf("CPU Burst: Parent (%d) -> Child (%d)\n", getpid(), target_pid);
    fprintf(fd, "CPU Burst: Parent (%d) -> Child (%d)\n", getpid(), target_pid);

    // Send child a signal SIGUSR1
    kill(target_pid, SIGALRM);

    p->burst -= 1;

    if(p->burst == 0){ // In RunQ, consume all burst time
        head++;
        p = pop_queue(&runq_t);
        printf("Remain burst time: %d\n", 0);
        fprintf(fd, "Remain burst time: %d\n", 0);
        free(p);
    } else if(p->time_quantum == 0){ // In RunQ, consume all time quantum
        head++;
        p = pop_queue(&runq_t);
        add_queue(&runq_t, p->pid, p->burst, Time_Q-1);
        burst = p->burst;

        printf("Remain burst time: %d\n", burst);
        fprintf(fd, "Remain burst time: %d\n", burst);
    } else{  // In RunQ, remain both time quantum and burst time
        p->time_quantum -= 1;
        burst = p->burst;

        printf("Remain burst time: %d\n", burst);
        fprintf(fd, "Remain burst time: %d\n", burst);
    }

    struct Proc_t* temp = malloc(sizeof(struct Proc_t));
    temp = runq_t.front;
    printf("Run Queue(%d): ", runq_t.count);
    fprintf(fd, "Run Queue(%d): ", runq_t.count);

    for(int i = 0; i < runq_t.count; i++){
        printf("[%d] ", temp->pid);
        fprintf(fd, "[%d] ", temp->pid);
        temp = temp->next;
    }

    printf("\n-----------------------------------------\n");
    fprintf(fd, "\n-----------------------------------------\n");

    if(count == total_burst_time){
        printf("[Program End]\n");
        fprintf(fd, "[Program End]\n");
        fclose(fd);
        exit(0);
    }
}

