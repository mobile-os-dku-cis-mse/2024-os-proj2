#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmm2.h"
#include "msg2.h"

// Global variables
MMU32 mmu32 = {0};
FrameInfo frames[NUM_FRAMES] = {0};
SwapEntry *swap_space = NULL;
uint64_t global_clock = 0;
extern PageDirectory *process_page_directories[MAX_CPROC];
static int fifo_next_victim = 1;  // 0번 프레임은 제외하고 시작

int get_free_frame(void) {
    for(int i = 1; i < NUM_FRAMES; i++) {
        if(!frames[i].is_used) {
            frames[i].is_used = 1;
            frames[i].last_access_time = ++global_clock;
            return i;
        }
    }
    return -1;
}

void *get_frame_address(int frame_number) {
    static char memory[NUM_FRAMES * FRAME_SIZE];
    return &memory[frame_number * FRAME_SIZE];
}

int find_fifo_victim(void) {
    int initial_position = fifo_next_victim;
    
    do {
        // 페이지 테이블로 사용되지 않는 프레임 찾기
        if (!frames[fifo_next_victim].is_page_table) {
            int victim = fifo_next_victim;
            fifo_next_victim = (fifo_next_victim + 1) % NUM_FRAMES;
            if (fifo_next_victim == 0) fifo_next_victim = 1; // 0번 프레임 건너뛰기
            return victim;
        }
        fifo_next_victim = (fifo_next_victim + 1) % NUM_FRAMES;
        if (fifo_next_victim == 0) fifo_next_victim = 1; // 0번 프레임 건너뛰기
    } while (fifo_next_victim != initial_position);
    
    return -1;  // 사용 가능한 프레임을 찾지 못함
}

int find_lru_victim(void) {
    uint64_t oldest_time = global_clock;
    int victim_frame = -1;

    for (int i = 1; i < NUM_FRAMES; i++) {
        if (!frames[i].is_page_table && frames[i].last_access_time < oldest_time) {
            oldest_time = frames[i].last_access_time;
            victim_frame = i;
        }
    }
    return victim_frame;
}

void init_paging_system(void) {
    memset(&mmu32, 0, sizeof(MMU32));
    memset(frames, 0, sizeof(frames));
    
    swap_space = malloc(sizeof(SwapEntry) * SWAP_SIZE);
    memset(swap_space, 0, sizeof(SwapEntry) * SWAP_SIZE);
    
    printf("[VMM] Initialized paging system\n");
}

uint32_t translate_address(uint32_t virtual_addr) {
    update_memory_access();
    if (!mmu32.current_pd) {
        mmu32.translation_result.valid = 0;
        strcpy(mmu32.translation_result.error_msg, "No active page directory");
        return 0;
    }

    uint32_t dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;

    page_dir_entry *dir_entry = &mmu32.current_pd->directory[dir_index];
    if (!dir_entry->present) {
        mmu32.translation_result.valid = 0;
        strcpy(mmu32.translation_result.error_msg, "Page directory entry not present");
        return 0;
    }

    page_table_entry *page_table = mmu32.current_pd->tables[dir_index];
    page_table_entry *page_entry = &page_table[page_index];

    if (!page_entry->present) {
        mmu32.translation_result.valid = 0;
        strcpy(mmu32.translation_result.error_msg, "Page not present");
        return 0;
    }

    frames[page_entry->frame_number].last_access_time = ++global_clock;
    page_entry->accessed = 1;

    uint32_t physical_addr = (page_entry->frame_number << 12) | offset;
    mmu32.translation_result.physical_addr = physical_addr;
    mmu32.translation_result.valid = 1;

    return physical_addr;
}

int handle_page_fault(int process_id, uint32_t virtual_addr) {
    update_page_fault();
    uint32_t page_number = virtual_addr >> 12;

    printf("[Page Fault] Process %d, Page %u\n", process_id, page_number);
    
    // 먼저 빈 프레임을 찾아봄
    int frame = get_free_frame();
    
    // 빈 프레임이 없으면 교체 정책에 따라 희생자 선택
    if (frame == -1) {
        update_page_replacement();  // 교체 발생 카운트
        
        // 교체할 페이지 선택
        if (current_algorithm == ALGORITHM_FIFO) {
            frame = find_fifo_victim();
        } else {
            frame = find_lru_victim();
        }
        
        if (frame == -1) {
            printf("[Page Fault] No frames available\n");
            return -1;
        }
        
        // 선택된 victim 프레임의 현재 내용을 스왑아웃
        if (frames[frame].is_used) {
            printf("[Memory] Allocated frame %d\n", frame);
            // 스왑 공간 할당
            int swap_slot = find_free_swap_slot();
            if (swap_slot != -1) {
                // 현재 프레임 내용을 스왑 영역에 저장
                swap_space[swap_slot].process_id = frames[frame].process_id;
                swap_space[swap_slot].page_number = frames[frame].virtual_page;
                swap_space[swap_slot].is_used = 1;
                memcpy(swap_space[swap_slot].data,
                       get_frame_address(frame),
                       PAGE_SIZE);
                
                // 원래 페이지의 페이지 테이블 엔트리 업데이트
                PageDirectory *pd = process_page_directories[frames[frame].process_id];
                uint32_t dir_idx = frames[frame].virtual_page >> 10;
                uint32_t page_idx = frames[frame].virtual_page & 0x3FF;
                pd->tables[dir_idx][page_idx].present = 0;
                
                frames[frame].is_used = 0;
                update_swap_stats(0);  // swap out 기록
                printf("[Debug] Swapped out page %u of process %d to slot %d\n", 
                       frames[frame].virtual_page, frames[frame].process_id, swap_slot);
            }
        }
    }

    // 스왑된 페이지가 있는지 확인
    int swap_index = find_in_swap(process_id, page_number);
    if (swap_index != -1) {
        memcpy(get_frame_address(frame),
               swap_space[swap_index].data,
               PAGE_SIZE);
        swap_space[swap_index].is_used = 0;
        update_swap_stats(1);  // swap in 기록
        printf("[Debug] Swapped in page %u from slot %d\n", page_number, swap_index);
    }

    frames[frame].is_used = 1;
    frames[frame].process_id = process_id;
    frames[frame].virtual_page = page_number;
    frames[frame].last_access_time = ++global_clock;

    printf("[Page Fault] Resolved: Page %u mapped to frame %d\n", 
           page_number, frame);
    
    return frame;
}

void init_process_memory(int pid) {
    process_page_directories[pid] = (PageDirectory*)malloc(sizeof(PageDirectory));
    memset(process_page_directories[pid], 0, sizeof(PageDirectory));
    
    for(int i = 0; i < PAGE_DIR_SIZE; i++) {
        int frame = get_free_frame();
        if(frame != -1) {
            process_page_directories[pid]->directory[i].present = 1;
            process_page_directories[pid]->directory[i].pt_frame = frame;
            frames[frame].is_page_table = 1;
            
            process_page_directories[pid]->tables[i] = 
                (page_table_entry*)malloc(sizeof(page_table_entry) * PAGE_TABLE_SIZE);
            memset(process_page_directories[pid]->tables[i], 0, 
                   sizeof(page_table_entry) * PAGE_TABLE_SIZE);
        }
    }
}

void handle_memory_request(struct mem_msgbuf *mem_msg) {
    mmu32.current_pd = process_page_directories[mem_msg->process_index];
    
    printf("\n[Memory] Processing requests for Process %d\n", mem_msg->process_index);
    for(int i = 0; i < 10; i++) {
        uint32_t vaddr = mem_msg->virtual_addresses[i];
        
        // MMU 주소 변환 시도
        printf("\n[MMU] Translating virtual address 0x%08x\n", vaddr);
        printf("Page Number: %u\n", vaddr >> 12);
        printf("Offset: 0x%03x\n", vaddr & 0xFFF);
        
        uint32_t paddr = translate_address(vaddr);
        if(!mmu32.translation_result.valid) {
            printf("Page fault occurred: %s\n", mmu32.translation_result.error_msg);
            
            int new_frame = handle_page_fault(mem_msg->process_index, vaddr);
            if(new_frame >= 0) {
                uint32_t dir_idx = (vaddr >> 22) & 0x3FF;
                uint32_t page_idx = (vaddr >> 12) & 0x3FF;
                
                page_table_entry *pte = &mmu32.current_pd->tables[dir_idx][page_idx];
                pte->present = 1;
                pte->frame_number = new_frame;
                pte->accessed = 1;
                
                // 페이지 폴트 해결 후 다시 변환 시도
                printf("\n[MMU] Translating virtual address 0x%08x\n", vaddr);
                printf("Page Number: %u\n", vaddr >> 12);
                printf("Offset: 0x%03x\n", vaddr & 0xFFF);
                
                paddr = translate_address(vaddr);
            }
        }
        
        if(mmu32.translation_result.valid) {
            printf("[Memory Access] Successful\n");
            printf("Virtual Address: 0x%08x\n", vaddr);
            printf("Physical Address: 0x%08x\n", paddr);
            printf("Frame Number: %d\n", paddr >> 12);
            printf("Offset: 0x%03x\n", paddr & 0xFFF);
        }
    }
}