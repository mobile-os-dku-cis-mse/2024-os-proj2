#include "log_manager.h"

// 관리 변수
static int tlb_hit = 0;
static int tlb_miss = 0;
static int page_fault = 0;
static int page_hit = 0;
static int process_count = 0;
static int swap_in_count = 0;
static int swap_out_count = 0;

// 초기화 함수
void init_logmanager() {
    tlb_hit = 0;
    tlb_miss = 0;
    page_fault = 0;
    page_hit = 0;
    process_count = 0;
    swap_in_count = 0;
    swap_out_count = 0;
}

// 증가 함수
void increment_tlb_hit() {
    tlb_hit++;
}

void increment_tlb_miss() {
    tlb_miss++;
}

void increment_page_fault() {
    page_fault++;
}

void increment_page_hit() {
    page_hit++;
}

void increment_process_count() {
    process_count++;
}

void increment_swap_in() {
    swap_in_count++;
}

void increment_swap_out() {
    swap_out_count++;
}

// 로그 요약 출력 함수
void print_log_summary() {
    int total_tlb_access = tlb_hit + tlb_miss;
    int total_page_access = page_hit + page_fault;

    printf("\n========= LOG SUMMARY =========\n");

    // TLB Summary
    printf("TLB Hit: %d (%.3f%%)\n", tlb_hit, 
           total_tlb_access > 0 ? (tlb_hit * 100.0 / total_tlb_access) : 0.0);
    printf("TLB Miss: %d (%.3f%%)\n", tlb_miss, 
           total_tlb_access > 0 ? (tlb_miss * 100.0 / total_tlb_access) : 0.0);

    // Page Summary
    printf("Page Hit: %d (%.3f%%)\n", page_hit, 
           total_page_access > 0 ? (page_hit * 100.0 / total_page_access) : 0.0);
    printf("Page Fault: %d (%.3f%%)\n", page_fault, 
           total_page_access > 0 ? (page_fault * 100.0 / total_page_access) : 0.0);

    // Swap Summary
    printf("Swap In: %d\n", swap_in_count);
    printf("Swap Out: %d\n", swap_out_count);

    // Process Summary
    printf("Processes Managed: %d\n", process_count);

    printf("===============================\n\n");
}