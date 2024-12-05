// vmm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vmm.h"

// 전역 변수 정의
PageTable page_tables[QUEUE_SIZE];
PhysicalMemory pmem;
free_frame_list frame_list;
MMU mmu;
int current_time = 0;
PerformanceMetrics metrics = {0};

// 물리 메모리 초기화
void init_physical_memory() {
    memset(&pmem, 0, sizeof(PhysicalMemory));
    for(int i = 0; i < NUM_FRAMES; i++) {
        pmem.frames[i].is_used = 0;
        pmem.frames[i].owner_pid = -1;
        pmem.frames[i].owner_page = -1;
        pmem.frames[i].allocation_time = 0;
    }
    printf("[Memory] Physical memory initialized\n");
}

// Free Frame List 초기화
void init_free_frame_list() {
    frame_list.front = NULL;
    frame_list.rear = NULL;
    frame_list.count = 0;

    // 프레임 1부터 15까지 추가 (0번은 커널용)
    for(int i = 1; i < NUM_FRAMES; i++) {
        frame_node* new_node = (frame_node*)malloc(sizeof(frame_node));
        new_node->frame_number = i;
        new_node->next = NULL;

        if(frame_list.rear == NULL) {
            frame_list.front = new_node;
            frame_list.rear = new_node;
        } else {
            frame_list.rear->next = new_node;
            frame_list.rear = new_node;
        }
        frame_list.count++;
    }
    printf("[Memory] Free Frame List initialized with %d frames\n", frame_list.count);
}

// MMU 초기화
void init_mmu() {
    mmu.ptbr = -1;
    mmu.translation_result.is_valid = 0;
    printf("[MMU] MMU initialized\n");
}

// 비어있는 프레임 가져오기
int get_free_frame() {
    if(frame_list.count == 0) {
        printf("[Memory] No free frames available\n");
        return -1;
    }

    frame_node* node = frame_list.front;
    int frame = node->frame_number;
    
    frame_list.front = node->next;
    if(frame_list.front == NULL) {
        frame_list.rear = NULL;
    }
    
    free(node);
    frame_list.count--;
    
    printf("[Memory] Allocated frame %d. Remaining free frames: %d\n", 
           frame, frame_list.count);
    return frame;
}

// 프레임 반환
void return_frame(int frame_number) {
    frame_node* new_node = (frame_node*)malloc(sizeof(frame_node));
    new_node->frame_number = frame_number;
    new_node->next = NULL;

    if(frame_list.rear == NULL) {
        frame_list.front = new_node;
        frame_list.rear = new_node;
    } else {
        frame_list.rear->next = new_node;
        frame_list.rear = new_node;
    }
    frame_list.count++;
    printf("[Memory] Returned frame %d to free list\n", frame_number);
}

// MMU의 주소 변환
int mmu_translate_address(unsigned int virtual_addr) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    metrics.total_memory_accesses++;
    metrics.process_stats[mmu.ptbr].memory_accesses++;

    unsigned int page_number = virtual_addr >> 12;
    unsigned int offset = virtual_addr & 0xFFF;

    printf("\n[MMU] Translating virtual address 0x%04x\n", virtual_addr);
    printf("├── Page Number: %d\n", page_number);
    printf("└── Offset: 0x%03x\n", offset);

    if(mmu.ptbr == -1) {
        mmu.translation_result.is_valid = 0;
        strcpy(mmu.translation_result.error_msg, "Invalid PTBR");
        return -1;
    }

    page_table_entry* pte = &page_tables[mmu.ptbr].entries[page_number];
    
    if(!pte->valid_bit) {
        mmu.translation_result.is_valid = 0;
        strcpy(mmu.translation_result.error_msg, "Page Fault: Invalid Page");
        return -1;
    }

    mmu.translation_result.frame_number = pte->frame_number;
    mmu.translation_result.offset = offset;
    mmu.translation_result.is_valid = 1;

    clock_gettime(CLOCK_MONOTONIC, &end);
    metrics.total_address_translation_time += 
        (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    metrics.address_translations++;

    return (pte->frame_number << 12) | offset;
}

// 페이지 폴트 처리
void handle_page_fault(int process_idx, unsigned int page_number) {
    metrics.page_faults++;
    metrics.process_stats[process_idx].page_faults++;

    printf("[Page Fault] Process %d, Page %d\n", process_idx, page_number);

    int new_frame = get_free_frame();
    if(new_frame == -1) {
        // FIFO로 희생 프레임 선택
        int oldest_time = current_time;
        int victim_frame = -1;
        
        for(int i = 1; i < NUM_FRAMES; i++) {
            if(pmem.frames[i].allocation_time < oldest_time) {
                oldest_time = pmem.frames[i].allocation_time;
                victim_frame = i;
            }
        }

        metrics.page_replacements++;
        printf("[Page Replacement] Selecting victim frame %d\n", victim_frame);

        // 이전 페이지 테이블 엔트리 무효화
        int old_pid = pmem.frames[victim_frame].owner_pid;
        int old_page = pmem.frames[victim_frame].owner_page;
        page_tables[old_pid].entries[old_page].valid_bit = 0;

        new_frame = victim_frame;
    }

    // 새 프레임 할당
    pmem.frames[new_frame].is_used = 1;
    pmem.frames[new_frame].owner_pid = process_idx;
    pmem.frames[new_frame].owner_page = page_number;
    pmem.frames[new_frame].allocation_time = current_time;

    // 페이지 테이블 엔트리 업데이트
    page_tables[process_idx].entries[page_number].frame_number = new_frame;
    page_tables[process_idx].entries[page_number].valid_bit = 1;
    page_tables[process_idx].entries[page_number].referenced_bit = 1;
    
    printf("[Page Fault] Resolved: Page %d mapped to frame %d\n", 
           page_number, new_frame);
}

// 메모리 접근 처리
void handle_memory_access(unsigned int virtual_addr) {
    int physical_addr = mmu_translate_address(virtual_addr);
    
    if(!mmu.translation_result.is_valid) {
        printf("[MMU] Address translation failed: %s\n", 
               mmu.translation_result.error_msg);
        handle_page_fault(mmu.ptbr, virtual_addr >> 12);
        physical_addr = mmu_translate_address(virtual_addr);
    }

    if(mmu.translation_result.is_valid) {
        printf("[Memory Access] Successful\n");
        printf("├── Virtual Address: 0x%04x\n", virtual_addr);
        printf("├── Physical Address: 0x%04x\n", physical_addr);
        printf("├── Frame Number: %d\n", mmu.translation_result.frame_number);
        printf("└── Offset: 0x%03x\n", mmu.translation_result.offset);
    }
}

// 메모리 상태 출력
void print_memory_status() {
    printf("\n===== Memory Status =====\n");
    for(int i = 0; i < NUM_FRAMES; i++) {
        printf("Frame %2d: ", i);
        if(pmem.frames[i].is_used) {
            printf("Used by Process %d, Page %d (Time: %d)\n",
                   pmem.frames[i].owner_pid,
                   pmem.frames[i].owner_page,
                   pmem.frames[i].allocation_time);
        } else {
            printf("Free\n");
        }
    }
    printf("Free Frames: %d\n", frame_list.count);
    printf("=====================\n\n");
}

// 성능 지표 출력
void print_performance_metrics() {
    printf("\n===== Performance Metrics =====\n");//전체 성능 지표 확인
    printf("Total Memory Accesses: %d\n", metrics.total_memory_accesses); //메모리 접근 횟수
    printf("Total Page Faults: %d\n", metrics.page_faults);
    printf("Page Fault Rate: %.2f%%\n", //페이지 폴트 비율
           (float)metrics.page_faults * 100 / metrics.total_memory_accesses);
    printf("Page Replacements: %d\n", metrics.page_replacements); //페이지 교체 횟수
    printf("Average Address Translation Time: %.2f ns\n", //평균 주소 변환 소요 시간
           (float)metrics.total_address_translation_time / metrics.address_translations);
    
    printf("\nPer-Process Statistics:\n");//프로세스 당 성능 지표 확인
    for(int i = 0; i < MAX_CPROC; i++) {
        if(metrics.process_stats[i].memory_accesses > 0) {
            printf("Process %d:\n", i);
            printf("├── Memory Accesses: %d\n", metrics.process_stats[i].memory_accesses);
            printf("├── Page Faults: %d\n", metrics.process_stats[i].page_faults);
            printf("└── Fault Rate: %.2f%%\n",
                   (float)metrics.process_stats[i].page_faults * 100 / 
                   metrics.process_stats[i].memory_accesses);
        }
    }
    printf("============================\n\n");
}