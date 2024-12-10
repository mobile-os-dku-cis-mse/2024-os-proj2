#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <stdint.h>        // uint32_t 등의 자료형
#include <signal.h>        // signal 관련 함수
#include <sys/types.h>     // pid_t 정의
#include <stdio.h>         // 입출력 함수
#include <stdlib.h>        // 일반 유틸리티 함수
#include <sys/time.h>      // 타이머 관련 함수
#include <sys/signal.h>
#include <unistd.h>
#include <time.h>
#include "memory_manager.h"
#include "ipc_manager.h"
#include "queue.h"
#include "log_manager.h"

// CPU 구조체 정의
typedef struct {
    int pid;
    FirstPageTable* page_table;  // 페이지 테이블
    uint32_t current_va;        // 현재 가상 주소
    int is_waiting;             // 대기 상태 플래그
    FILE* data_file;            // 프로세스 전용 데이터 파일 포인터 (status 4 전용)
} CPU;

void process_manager(char* tar_name, int time, int page_r, int tlb_c, int page_s, int memory_s, int tlb_s, int status, int random);

#endif // PROCESS_MANAGER_H


// #ifndef PROCESS_MANAGER_H
// #define PROCESS_MANAGER_H

// #include <stdint.h>        // uint32_t 등의 자료형
// #include <signal.h>        // signal 관련 함수
// #include <sys/types.h>     // pid_t 정의
// #include <stdio.h>         // 입출력 함수
// #include <stdlib.h>        // 일반 유틸리티 함수
// #include <sys/time.h>      // 타이머 관련 함수
// #include <sys/signal.h>
// #include <unistd.h>
// #include <time.h>
// #include "memory_manager.h"
// #include "ipc_manager.h"
// #include "queue.h"
// #include "log_manager.h"

// // 상수 정의
// #define NUM_PROCESSES 10   // 프로세스 수 정의
// #define TIME_QUANTUM 5

// // CPU 구조체 정의
// typedef struct {
//     int status;     // CPU 상태 (1: Read, 2: Write, 3: Mixed 등)
//     int random;     // Random 여부 (0: 순차, 1: 랜덤)
//     uint32_t pc;    // Program Counter (가상 주소)
// } CPU;


// // 시그널 핸들러 함수 선언
// void alrm_handler(int signo);          // SIGALRM 핸들러
// void sigusr1_handler(int signo);       // SIGUSR1 핸들러

// // 타이머 설정 함수
// void setup_timer();

// // 프로세스 초기화 및 관리 함수
// void process_manager(char* tar_name, int time_limit, int page_r, int tlb_c, int page_s,
//                      int memory_s, int tlb_s, int status, int random);
// void parent_process_scheduler(int time_limit, int status, int random);

// // 자식 프로세스 작업 처리 함수
// void child_process(int process_id, FirstPageTable* page_table);

// #endif // PROCESS_MANAGER_H
