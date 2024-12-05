// vmm.h
#ifndef VMM_H
#define VMM_H

#include "msg.h"

#define MAX_CPROC 10        // 최대 프로세스 수

// Free Frame List 노드
typedef struct frame_node {
    int frame_number;
    struct frame_node* next;
} frame_node;

// Free Frame List
typedef struct {
    frame_node* front;
    frame_node* rear;
    int count;
} free_frame_list;

// 프레임 정보
typedef struct {
    int is_used;            // 프레임 사용 여부
    int owner_pid;          // 이 프레임을 사용중인 프로세스 ID
    int owner_page;         // 이 프레임에 매핑된 페이지 번호
    int allocation_time;    // FIFO를 위한 프레임 할당 시간
} frame_info;

// 물리 메모리 관리 구조체
typedef struct {
    char memory[NUM_FRAMES][FRAME_SIZE];  // 실제 물리 메모리
    frame_info frames[NUM_FRAMES];        // 각 프레임의 정보
} PhysicalMemory;

// MMU 구조체
typedef struct {
    int ptbr;     // Page Table Base Register

    // 주소 변환 결과
    struct {
        int frame_number;
        int offset;
        int is_valid;
        char error_msg[100];
    } translation_result;
} MMU;

typedef struct {
    // 페이지 폴트 관련
    int total_memory_accesses;           // 전체 메모리 접근 횟수
    int page_faults;                     // 전체 페이지 폴트 횟수
    int page_replacements;               // 페이지 교체 횟수
    
    // 프로세스별 통계
    struct {
        int memory_accesses;             // 메모리 접근 횟수
        int page_faults;                 // 페이지 폴트 횟수
    } process_stats[MAX_CPROC];
    
    // 시간 관련
    long total_address_translation_time;  // 주소 변환에 걸린 총 시간
    int address_translations;            // 주소 변환 시도 횟수
    
    // 메모리 사용 관련
    int current_page_table_size;         // 현재 페이지 테이블이 사용 중인 공간
} PerformanceMetrics;



// 전역 변수 선언
extern PageTable page_tables[QUEUE_SIZE];  // 프로세스당 페이지 테이블
extern PhysicalMemory pmem;
extern free_frame_list frame_list;
extern MMU mmu;
extern int current_time;  // 현재 시스템 시간
extern PerformanceMetrics metrics;

// 함수 선언
void init_physical_memory();
void init_free_frame_list();
void init_mmu();
int get_free_frame();
void return_frame(int frame_number);
int mmu_translate_address(unsigned int virtual_addr);
void handle_page_fault(int process_idx, unsigned int page_number);
void handle_memory_access(unsigned int virtual_addr);
void print_memory_status();
void print_performance_metrics();
#endif