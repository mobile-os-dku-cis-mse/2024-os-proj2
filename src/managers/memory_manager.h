#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

// library
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// custom header
#include "memory.h"
#include "page_table.h"
#include "tlb.h"
#include "log_manager.h"

// 전역 변수 선언
extern int time_tick;
extern int page_replace;   // FIFO, LRU, LFU, SECOND_CHANCE (0, 1, 2, 3)
extern int tlb_cache;      // RANDOM, FIFO, LRU, LFU (0,1,2,3)
extern int page_size;      // 페이지 크기
extern int memory_size;    // 메모리 크기
extern int tlb_size;       // TLB 크기
extern int frame_count;    // 프레임 개수
extern uint32_t access_counter; // VA 접근 횟수 누적
extern FILE* write_fp;


// 함수 프로토타입
FirstPageTable* init_memory_manager(int time, int page_r, int tlb_c, int page_s, int memory_s , int tlb_s);
int search_tlb(uint32_t va);
int is_memory_full();
void update_tlb(uint32_t va, uint32_t frame_number, uint8_t read_only);
int select_victim();
int find_disk_block();
void swap_in(FirstPageTable* page_table,int src_disk_block, int dst_memory_frame);
void swap_out(FirstPageTable* page_table,int src_memory_frame, int dst_disk_block, int status);
char read_va(FirstPageTable* page_table,uint32_t va);
void write_va(FirstPageTable* page_table,uint32_t va, char value);
void initial_allocate(FirstPageTable* page_table,uint32_t va, char value);
void init_tlb_entry(TLBEntry* entry);
void free_memory_manager();
void dump_page_table(FirstPageTable* page_table);

#endif // PROCESS_MANAGER_H
