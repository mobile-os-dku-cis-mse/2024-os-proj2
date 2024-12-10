#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdio.h>

// 초기화 함수
void init_logmanager();

// 증가 함수
void increment_tlb_hit();
void increment_tlb_miss();
void increment_page_fault();
void increment_page_hit();
void increment_process_count();
void increment_swap_in();
void increment_swap_out();

// 출력 함수
void print_log_summary();

#endif // LOG_MANAGER_H
