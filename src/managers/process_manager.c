#include "process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

// 전역 변수
#define NUM_PROCESSES 10
CPU processes[NUM_PROCESSES];
int current_time = 0; // 타이머 틱
FILE* log;
uint32_t virtual_address;

// 타이머 틱 함수
void timer_tick() {
    current_time++;
    fprintf(log, "\n\n================== Timer tick  %d===================\n\n", current_time);
}

// 프로세스 작업 처리 함수
void process_task(CPU* process, int status, int random, uint32_t base_va) {
    if (status == 4) { // 파일에서 작업 수행
        char line[128];
        if (fgets(line, sizeof(line), process->data_file)) {
            uint32_t va;
            int rw_flag;
            char value;

            // 파일 내용 파싱
            int items_read = sscanf(line, "0x%x %d %c", &va, &rw_flag, &value);

            // 읽기 작업 수행
            if (rw_flag == 0) { // Read
                char data = read_va(process->page_table, va);
                fprintf(log, "[Process %d] Read Data: '%c' at VA=0x%X\n", process->pid, data, va);
            } else if (rw_flag == 1 && items_read == 3) { // Write
                write_va(process->page_table, va, value);
                fprintf(log, "[Process %d] Written Data: '%c' at VA=0x%X\n", process->pid, value, va);
            } else {
                fprintf(log, "[Process %d] Invalid line in file or unsupported format: %s", process->pid, line);
            }
        } else {
            fprintf(log, "[Process %d] End of file reached. No more tasks.\n", process->pid);
        }
        return;
    }

    // Random이 0인 경우, 각 프로세스는 주어진 base_va로 작업 수행
    if (random == 0) {
        process->current_va = process->pid; // 각 프로세스는 고유한 VA 할당
    } else {
        process->current_va = rand() % virtual_address; // Random 모드에서는 랜덤 VA 사용
    }

    // 읽기/쓰기 작업 수행
    if (status == 1) { // Read 작업
        char data = read_va(process->page_table, process->current_va);
        fprintf(log, "[Process %d] Read Data: '%c' at VA=0x%X\n", process->pid, data, process->current_va);
    } else if (status == 2) { // Write 작업
        write_va(process->page_table, process->current_va, 'A');
        fprintf(log, "[Process %d] Written Data: 'A' at VA=0x%X\n", process->pid, process->current_va);
    } else if (status == 3) { // 읽기와 쓰기 교대 작업
        if (current_time % 2 == 0) {
            char data = read_va(process->page_table, process->current_va);
            fprintf(log, "[Process %d] Read Data: '%c' at VA=0x%X\n", process->pid, data, process->current_va);
        } else {
            write_va(process->page_table, process->current_va, 'B');
            fprintf(log, "[Process %d] Written Data: 'B' at VA=0x%X\n", process->pid, process->current_va);
        }
    }
}

// 부모 프로세스 스케줄러
void parent_process_scheduler(int time_limit, int status, int random, uint32_t virtual_memory_size) {
    fprintf(log, "[Parent] Starting Process Scheduler...\n");

    for (int tick = 0; tick < time_limit; tick++) {
        timer_tick();

        // Random이 0일 경우, 현재 타임 틱에 따라 Base VA 계산
        uint32_t base_va = (random == 0) ? 0 : virtual_address;

        for (int i = 0; i < NUM_PROCESSES; i++) {
            CPU* current_process = &processes[i];
            process_task(current_process, status, random, base_va);
        }

        fprintf(log, "[Parent] All tasks completed for time tick %d.\n", tick + 1);
    }
}

// 메모리 초기화 및 프로세스 관리 함수
void process_manager(char* tar_name, int time, int page_r, int tlb_c, int page_s, int memory_s, int tlb_s, int status, int random) {
    log = fopen("process.txt","w");
    fprintf(log, "[Parent] Initializing Process Manager...\n");
    struct timeval start, end;

    // 메모리 매니저 초기화
    FirstPageTable* page_table = init_memory_manager(time, page_r, tlb_c, page_s, memory_s, tlb_s);

    fprintf(log, "[Parent] Memory Manager and Log Manager initialized.\n");

    // 가상 메모리 로드
    FILE* tar_file = fopen(tar_name, "rb");
    if (!tar_file) {
        perror("[Parent] Failed to open tar file");
        exit(EXIT_FAILURE);
    }

    char line_buffer[1024];
    virtual_address = 0;

    while (fgets(line_buffer, sizeof(line_buffer), tar_file)) {
        for (int i = 0; line_buffer[i] != '\0'; i++) {
            initial_allocate(page_table, virtual_address, line_buffer[i]);
            virtual_address++;
        }
    }
    fclose(tar_file);
    fprintf(log, "[Parent] Virtual memory loaded. Total VA=0x%X, %d\n", virtual_address, page_s);


    // 각 프로세스 초기화
    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].pid = i;
        processes[i].page_table = (FirstPageTable*)malloc(sizeof(FirstPageTable));
        copy_page_table(page_table, processes[i].page_table);
        processes[i].current_va = 0;

        // 데이터 파일 열기 (status 4 전용)
        if (status == 4) {
            char filename[50];
            snprintf(filename, sizeof(filename), "pre_process_data/%dkb/data_%d.txt", page_s, i + 1);
            processes[i].data_file = fopen(filename, "r");
            if (!processes[i].data_file) {
                perror("[Parent] Failed to open data file");
                exit(EXIT_FAILURE);
            }
        }

        fprintf(log, "[Parent] Process %d initialized.\n", i);
    }

    //dump_page_table(processes[1].page_table);

    init_logmanager();

    // 스케줄러 실행
    gettimeofday(&start, NULL);
    parent_process_scheduler(time, status, random, virtual_address);
    gettimeofday(&end, NULL);


    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;



    fprintf(log, "[Parent] All processes completed.\n\n");
    print_log_summary();
    printf("\n\nElapsed time: %.6f seconds\n", elapsed);
    fclose(log);

    // 자원 해제
    for (int i = 0; i < NUM_PROCESSES; i++) {
        free(processes[i].page_table);
        if (status == 4 && processes[i].data_file) {
            fclose(processes[i].data_file);
        }
    }
    free_memory_manager();
}





// //========================================================================================
// #include "process_manager.h"
// #include "ipc_manager.h"
// #include "queue.h"
// #include "log_manager.h"
// #include "memory_manager.h"

// #define TIME_QUANTUM 5

// // 전역 변수
// pid_t child_pids[NUM_PROCESSES];
// Queue* ready_queue;
// int msgid;
// CPU* cpu;
// uint32_t virtual_address;
// volatile sig_atomic_t completed_children = 0;

// void setup_timer() {
//     struct sigaction sa;
//     struct itimerval timer;

//     // 타이머가 실행될 때마다 호출될 신호 핸들러 설정
//     sa.sa_handler = alrm_handler;
//     sigemptyset(&sa.sa_mask);
//     sa.sa_flags = SA_RESTART; // 타이머를 자동 재설정
//     sigaction(SIGALRM, &sa, NULL);

//     // 타이머 설정: TIME_QUANTUM마다 SIGALRM 발생
//     timer.it_value.tv_sec = 0;
//     timer.it_value.tv_usec = TIME_QUANTUM * 1000; // TIME_QUANTUM을 마이크로초로 변환
//     timer.it_interval = timer.it_value; // 반복 간격 설정
//     setitimer(ITIMER_REAL, &timer, NULL); // 실제 타이머 설정
// }


// // SIGCONT 신호를 처리하는 핸들러
// void handle_sigcont(int sig)
// {
//     // 단순히 신호 처리를 위한 빈 핸들러
// }

// // SIGALRM 핸들러 (타이머 틱 발생)
// void alrm_handler(int signo) {
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         kill(child_pids[i], SIGCONT); // 자식 프로세스를 SIGCONT로 깨움
//     }
// }

// // SIGUSR1 핸들러 (자식 → 부모, 작업 완료 알림)
// void sigusr1_handler(int signo) {
//     completed_children++;
//     fprintf(log, "[Parent] SIGUSR1 received. Completed children: %d/%d\n", completed_children, NUM_PROCESSES);
// }

// // SIGUSR2 핸들러 (부모 → 자식, 작업 시작 알림)
// void sigusr2_handler(int signo) {
//     // 단순히 자식 프로세스를 깨움
// }

// // 자식 프로세스 작업
// void child_process(int process_id, FirstPageTable* page_table) {
//     fprintf(log, "[Child %d] Initialized and received page table.\n", process_id);

//     // SIGCONT 핸들러 설정
//     signal(SIGCONT, handle_sigcont);

//     while (1) {
//         pause(); // SIGCONT 신호 대기

//         // 메시지 수신 루프
//         Message* msg = (Message *)malloc(sizeof(Message));
//         msgrcv(msgid, msg, sizeof(Message) - sizeof(long), getpid(), IPC_NOWAIT);
    
//         fprintf(log, "[Child %d] Processing task: VA=0x%X, Read/Write=%d\n", process_id, msg->va, msg->read_write);

//         // 작업 처리
//         if (msg->read_write == 0) { // Read 작업
//             char data = read_va(page_table, msg->va);
//             fprintf(log, "[Child %d] Read Data: '%c' at VA=0x%X\n", process_id, data, msg->va);
//         } else if (msg->read_write == 1) { // Write 작업
//             write_va(page_table, msg->va, msg->value);
//             fprintf(log, "[Child %d] Written Data: '%c' at VA=0x%X\n", process_id, msg->value, msg->va);
//         }

//         // 부모에게 작업 완료 신호 전송
//         kill(getppid(), SIGUSR2);

//         free(msg);

//         fprintf(log, "[Child %d] Task completed and response sent.\n", process_id);
//     }
// }

// // 부모 프로세스 스케줄러
// void parent_process_scheduler(int time_limit, int status, int random) {
//     fprintf(log, "[Parent] Starting Process Scheduler...\n");

//     signal(SIGALRM, alrm_handler); // SIGALRM 신호를 타이머 핸들러에 연결
//     signal(SIGUSR1, sigusr1_handler); // SIGUSR1 신호를 처리할 핸들러 설정
//     signal(SIGUSR2, sigusr2_handler);

//     setup_timer();

//     uint32_t virtual_memory_size = virtual_address;

//     //setup_timer();
//     sleep(1);

//     for (int tick = 0; tick < time_limit; tick++) {

//         pause();
//         fprintf(log, "[Parent] Time tick %d start.\n", tick + 1);
//         completed_children = 0;
//         fprintf(log, "%d/%d\n",tick,time_limit);

//         for (int i = 0; i < NUM_PROCESSES; i++) {


//             fprintf(log, "dkdkdkd1 : %d\n",cpu->pc);
//             PCB* current_pcb = dequeue(ready_queue);

//             if (random == 1) {
//                 cpu->pc = rand() % virtual_memory_size;
//             }

//             Message msg = {
//                 current_pcb->pid,
//                 current_pcb->pid,
//                 i,
//                 (status == 1) ? '\0' : (status == 2) ? 'A' : (tick < time_limit / 2) ? '\0' : 'A',
//                 (status == 1) ? 0 : (status == 2) ? 1 : (tick < time_limit / 2) ? 0 : 1,
//                 cpu->status,
//                 cpu->pc};

//             sendMessage(msgid, &msg);
//             kill(current_pcb->pid, SIGCONT);
//             pause();

//             //PCB* current_pcb = dequeue(ready_queue);
//             cpu->pc++;
//             current_pcb->va = cpu->pc;
//             enqueue(ready_queue,current_pcb);


//             fprintf(log, "[Parent] Sending task to PID=%d: VA=0x%X, Read/Write=%d\n", msg.pid, msg.va, msg.read_write);

//             free(current_pcb);
        
//         }

//         fprintf(log, "[Parent] All tasks completed for time tick %d.\n", tick + 1);
//     }
// }

// // 부모 프로세스 관리
// void process_manager(char* tar_name, int time_limit, int page_r, int tlb_c, int page_s, int memory_s, int tlb_s, int status, int random) {
//     fprintf(log, "[Parent] Initializing Process Manager...\n");

//     // SIGUSR1 핸들러 설정
//     // struct sigaction sa_usr1;
//     // sa_usr1.sa_handler = sigusr1_handler;
//     // sa_usr1.sa_flags = SA_RESTART;
//     // sigemptyset(&sa_usr1.sa_mask);
//     // sigaction(SIGUSR1, &sa_usr1, NULL);

//     cpu = (CPU*)malloc(sizeof(CPU));
//     cpu->status = status;
//     cpu->random = random;
//     cpu->pc = 0;
    

//     ready_queue = createQueue();
//     msgid = createMessageQueue();
//     FirstPageTable* parent_page_table = init_memory_manager(time_limit, page_r, tlb_c, page_s, memory_s, tlb_s);
//     init_logmanager();

//     fprintf(log, "[Parent] Memory Manager and Log Manager initialized.\n");

//     // 가상 메모리 로드
//     FILE* tar_file = fopen(tar_name, "rb");
//     if (!tar_file) {
//         perror("[Parent] Failed to open tar file");
//         exit(EXIT_FAILURE);
//     }

//     char line_buffer[1024];
//     virtual_address = 0;
//     //
//     while (fgets(line_buffer, 1024, tar_file) != NULL) {
//         for (int i = 0; line_buffer[i] != '\0'; i++) {
//             initial_allocate(parent_page_table, virtual_address, line_buffer[i]);
//             virtual_address++;
//         }
//     }
//     fclose(tar_file);
//     fprintf(log, "[Parent] Virtual memory loaded. Total VA=0x%X\n", virtual_address);

//     // 페이지 테이블 복사
//     FirstPageTable* child_page_tables[NUM_PROCESSES];
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         child_page_tables[i] = (FirstPageTable*)malloc(sizeof(FirstPageTable));
//         copy_page_table(parent_page_table, child_page_tables[i]);
//         fprintf(log, "[Parent] Page table copied for child %d.\n", i);
//     }


//     //setup_timer(30); // 타이머 설정 (30ms)

//     // 자식 프로세스 생성
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         pid_t pid = fork();
//         if (pid == -1) {
//             perror("[Parent] Fork failed");
//             exit(EXIT_FAILURE);
//         } else if (pid == 0) {
//             child_process(i, child_page_tables[i]);
//             exit(EXIT_SUCCESS);
//         } else {
//             PCB* pcb = (PCB*)malloc(sizeof(PCB));
//             pcb->pid = pid;
//             pcb->read_write = status;
//             pcb->va = cpu->pc;
//             pcb->value = 'a';
//             enqueue(ready_queue, pcb);
//             child_pids[i] = pid;
//             fprintf(log, "[Parent] Child process %d created with PID=%d\n", i, pid);
//         }
//     }

//     parent_process_scheduler(time_limit, status, random);

//     fprintf(log, "[Parent] All child processes initialized.\n");
// }


//========================================================================================



// SIGUSR1 핸들러: 실행 중인 프로세스 교체
// void parent_sigusr1_handler(int signo) {
//     static volatile sig_atomic_t current_process = 0; // 현재 프로세스
//     current_process = (current_process + 1) % NUM_PROCESSES;
//     kill(child_pids[current_process], SIGUSR1); // 다음 프로세스 실행
// }

// // SIGINT 핸들러: 모든 프로세스 종료
// void parent_sigint_handler(int signo) {
//     fprintf(log, "[Parent] Stopping all child processes...\n");
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         kill(child_pids[i], SIGKILL);
//     }
//     while (wait(NULL) > 0);
//     fprintf(log, "[Parent] All child processes terminated. Exiting.\n");
//     cleanup_process_manager();
//     exit(0);
// }

// // 초기화 함수
// void init_process_manager(char* tar_name,int time, int page_r, int tlb_c, int page_s, int memory_s, int tlb_s, int status, int random) {
//     // CPU 초기화
//     cpu.status = status;
//     cpu.random = random;
//     cpu.pc = 0;

//     // IPC 초기화
//     //msgid = createMessageQueue();
//     // Memory Manager 초기화
//     init_memory_manager(time, page_r, tlb_c,page_s,memory_s,tlb_s);
//     //fprintf(log, "%s",tar_name);


//     char line_buffer[1024]; // 한 줄씩 읽어올 버퍼
//     FILE* tar_file = fopen(tar_name, "rb");
//     if (!tar_file) {
//         perror("Failed to open tar file");
//         exit(EXIT_FAILURE);
//     }
//     fprintf(log, "Reading tar file: %s\n", tar_name);

//     char buffer; // 읽은 데이터를 저장할 버퍼
//     virtual_address = 0x0;  // 가상 주소를 0x0에서 시작

//         // tar 파일에서 한 줄씩 읽기
//     while (fgets(line_buffer, 1024, tar_file) != NULL) {
//         for (int i = 0; line_buffer[i] != '\0'; i++) { // 한 줄의 각 문자를 처리
//             initial_allocate(virtual_address, line_buffer[i]); // 읽은 데이터를 메모리에 저장
//             virtual_address++; // 다음 가상 주소로 이동

//             // //실험 목적을 위한 가상 주소 제한
//             // if (virtual_address > 10000) {
//             //     return;
//             // }
//         }
//     }

//     fprintf(log, "virtual_mem : %d(0x%x)",virtual_address,  virtual_address);

//     fclose(tar_file);

//     fprintf(log, "Tar file loaded to memory.\n");

//     uint32_t finish_idx = virtual_address;

//     dump_page_table();
    
//     fprintf(log, "Page Table dumped to page_table_dump.txt\n");


//     uint32_t temp = 0;

//     init_logmanager();
//     for (uint32_t i = 0; i < time_tick; i++) {
//         uint32_t temp = (cpu.random == 1) ? rand()%virtual_address : i;

//         char temp2 = read_va(temp); // 가상 주소에서 읽기
//         //fprintf(log, "0x%x : %c\n", i,temp2);
//         // if (fprintf(fp, "%c", temp2) < 0) { // 읽은 데이터 텍스트 파일에 쓰기
//         //     perror("Failed to write to output file");
//         //     fclose(fp);
//         //     exit(EXIT_FAILURE);
//         // }
//     }
//     print_log_summary();


//     init_logmanager();
//     for (uint32_t i = 0; i < time_tick; i++) {
//         uint32_t temp = (cpu.random == 1) ? rand()%virtual_address : i;

//         write_va(temp, 'a'); // 가상 주소에서 읽기
//         //fprintf(log, "0x%x : %c\n", i,temp2);
//         // if (fprintf(fp, "%c", temp2) < 0) { // 읽은 데이터 텍스트 파일에 쓰기
//         //     perror("Failed to write to output file");
//         //     fclose(fp);
//         //     exit(EXIT_FAILURE);
//         // }
//     }
//     print_log_summary();




//     free_memory_manager();
//     // 페이지 교체 알고리즘 설정
//     //page_rep = page_replace;

//     // Ready Queue 초기화
//     //ready_queue = createQueue();

//     //fprintf(log, "[Parent] Process Manager Initialized.\n");
// }

// // 자식 프로세스 작업
// void child_process(int process_id) {
//     fprintf(log, "[Child %d] Ready to receive tasks.\n", process_id);
//     signal(SIGUSR1, SIG_DFL); // 기본 동작 설정

//     while (1) {
//         pause(); // SIGUSR1 신호 대기
//         fprintf(log, "[Child %d] Executing assigned task.\n", process_id);

//         // 메시지 수신 및 처리
//         Message msg;
//         receiveMessage(msgid, &msg, process_id + 1);

//         if (msg.read_write == 0) { // Read 작업
//             char data = read_memory(msg.va);
//             fprintf(log, "[Child %d] Read Data: '%c' at VA=0x%X\n", process_id, data, msg.va);
//         } else if (msg.read_write == 1) { // Write 작업
//             write_memory(msg.va, 'A'); // 예시 데이터 'A'
//             fprintf(log, "[Child %d] Written Data: 'A' at VA=0x%X\n", process_id, msg.va);
//         }

//         // 부모에게 작업 완료 신호 전송
//         msg.type = 1; // 부모 프로세스에게 신호
//         sendMessage(msgid, &msg);

//         fprintf(log, "[Child %d] Task completed.\n", process_id);
//     }
// }

// // 프로세스 매니저 실행
// void process_manager(int time_limit, int page_replace, int tlb_cache_policy, int page_size, int memory_size, int tlb_size) {
//     fprintf(log, "Running Process Manager...\n");

//     // 초기화
//     init_process_manager(time_limit, page_replace, tlb_cache_policy, page_size, memory_size, tlb_size);

//     // SIGUSR1 및 SIGINT 핸들러 등록
//     struct sigaction sa_usr1 = {0};
//     sa_usr1.sa_handler = parent_sigusr1_handler;
//     sigaction(SIGUSR1, &sa_usr1, NULL);

//     struct sigaction sa_int = {0};
//     sa_int.sa_handler = parent_sigint_handler;
//     sigaction(SIGINT, &sa_int, NULL);

//     // 자식 프로세스 생성
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         pid_t pid = fork();
//         if (pid == -1) {
//             perror("fork failed");
//             exit(EXIT_FAILURE);
//         } else if (pid == 0) {
//             child_process(i); // 자식 프로세스 작업 수행
//             exit(EXIT_SUCCESS);
//         } else {
//             PCB *pcb = (PCB *)malloc(sizeof(PCB));
//             pcb->pid = pid;
//             enqueue(ready_queue, pcb);
//         }
//     }

//     // 부모 프로세스: 작업 분배 및 스케줄링
//     for (int tick = 0; tick < time_limit; tick++) {
//         for (int i = 0; i < NUM_PROCESSES; i++) {
//             PCB *current_pcb = dequeue(ready_queue);
//             if (!current_pcb) {
//                 fprintf(log, "[Parent] No process in ready queue for child %d.\n", i);
//                 continue;
//             }

//             // 작업 메시지 생성 및 전송
//             Message msg;
//             msg.type = current_pcb->pid;
//             msg.pid = current_pcb->pid;
//             msg.va = cpu.pc;
//             msg.read_write = cpu.status;
//             sendMessage(msgid, &msg);

//             fprintf(log, "[Parent] Sent task to process %d.\n", i);
//             kill(current_pcb->pid, SIGUSR1);

//             enqueue(ready_queue, current_pcb);
//         }

//         // 자식 프로세스 완료 메시지 수신
//         for (int i = 0; i < NUM_PROCESSES; i++) {
//             Message completion_msg;
//             receiveMessage(msgid, &completion_msg, 1);
//             fprintf(log, "[Parent] Received completion message from process %d.\n", completion_msg.process_id);
//         }

//         fprintf(log, "[Parent] Time tick %d complete.\n", tick + 1);
//     }

//     cleanup_process_manager();
// }

// // 종료 함수
// void cleanup_process_manager() {
//     for (int i = 0; i < NUM_PROCESSES; i++) {
//         kill(child_pids[i], SIGKILL);
//         waitpid(child_pids[i], NULL, 0);
//     }
//     destroyQueue(ready_queue);

//     if (msgctl(msgid, IPC_RMID, NULL) == -1) {
//         perror("Failed to remove message queue");
//     }

//     fprintf(log, "Process Manager Cleaned Up.\n");
// }
