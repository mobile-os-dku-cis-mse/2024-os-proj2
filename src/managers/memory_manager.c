#include "memory_manager.h"

TLB* tlb;
//FirstPageTable* page_table;
MemoryMap* memory_map;
DiskMap* disk_map;
char* memory;
char* disk;

// 전역 변수 선언 (정의 아님)
uint32_t offset_bits;
uint32_t first_page;
uint32_t second_page;

int time_tick;
int page_replace;   // FIFO, LRU, LFU, SECOND_CHANCE (0, 1, 2, 3)
int tlb_cache;      // RANDOM, FIFO, LRU, LFU (0,1,2,3)
int page_size;      // 1, 2, 4, 16, 64
int memory_size;    // 16, 32, 64, 128
int tlb_size;       // 0, 32, 64, 128, 256, 512, 1024
int frame_count;    // frame 개수
FILE* write_fp;
uint32_t access_counter; // 전역 변수: VA 접근 횟수 누적



FirstPageTable* init_memory_manager(int time, int page_r, int tlb_c, int page_s, int memory_s , int tlb_s){

    time_tick = time;                       // 타임 틱
    page_replace = page_r;                  // 페이지 교체 정책
    tlb_cache = tlb_c;                      // tlb 교체 정책
    page_size = page_s * 1024;              // KB 단위
    memory_size = memory_s * 1024 * 1024;   // MB 단위
    tlb_size = tlb_s;
    frame_count = memory_size / page_size;

    PageConfig* pc = (PageConfig* )malloc(sizeof(PageConfig));
    // Page Table init
    FirstPageTable* page_table = init_page_table(pc ,page_size);
    
    // memory init
    memory = init_memory(memory_size);
    memory_map = init_memory_map(memory_size);

    offset_bits = pc->offset_bits;
    first_page = pc->first_page;
    second_page = pc->second_page;


    // disk init
    disk = init_disk();
    disk_map = init_disk_map(page_size);
    
    // tlb init
    tlb = init_tlb(tlb_size);   

    access_counter = 0;

    write_fp = fopen("detail_log.txt","w");

    return page_table;
}

int search_tlb(uint32_t va) {
    // TLB 검색을 위해 가상 주소를 분할
    uint32_t page_number = va >> offset_bits;

    // TLB에서 검색
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page_number == page_number) {
            // TLB 히트 처리
            //printf("TLB Hit: VA=0x%X -> Index=%d\n", va, i);
            tlb->entries[i].last_used = tlb->current_time++;
            return i; // 히트된 TLB 인덱스 반환
        }
    }

    // TLB 미스 처리
    //printf("TLB Miss: VA=0x%X\n", va);
    return -1; // 미스 시 -1 반환
}

int is_memory_full() {
    for (int i = 0; i < frame_count; i++) {
        if (memory_map[i].is_valid == 0) {
            // 사용 가능한 메모리 프레임이 있음
            return i;
        }
    }
    // 모든 메모리 프레임이 사용 중임
    return -1;
}

void update_tlb(uint32_t va, uint32_t frame_number, uint8_t read_only) {
    // 가상 주소에서 page_number 계산
    uint32_t page_number = va >> offset_bits; // va에서 offset_bits만큼 시프트하여 page_number 추출

    if(tlb_size == 0){
        return;
    }

    int replace_index = -1;

    // TLB에서 비어 있는 엔트리 확인 (full인지 체크)
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid == 0) {
            replace_index = i; // 비어 있는 엔트리 발견
            break;
        }
    }

    // TLB가 가득 찼다면 교체 정책 적용
    if (replace_index == -1) {
        if (tlb_cache == 0) { // RANDOM
            replace_index = rand() % tlb->size;
        } else if (tlb_cache == 1) { // FIFO
            replace_index = tlb->fifo_index;
            tlb->fifo_index = (tlb->fifo_index + 1) % tlb->size; // 순환 인덱스 갱신
        } else if (tlb_cache == 2) { // LRU
            uint32_t min_time = UINT32_MAX;
            for (int j = 0; j < tlb->size; j++) {
                if (tlb->entries[j].last_used < min_time) {
                    min_time = tlb->entries[j].last_used;
                    replace_index = j;
                }
            }
        } else if (tlb_cache == 3) { // LFU
            uint32_t min_count = UINT32_MAX;
            for (int j = 0; j < tlb->size; j++) {
                if (tlb->entries[j].access_count < min_count) {
                    min_count = tlb->entries[j].access_count;
                    replace_index = j;
                }
            }
        } else {
            //printf("Invalid TLB cache policy.\n");
            return;
        }

        // 교체할 엔트리 초기화
        init_tlb_entry(&tlb->entries[replace_index]);
    }

    // TLB 엔트리 갱신
    tlb->entries[replace_index].page_number = page_number;
    tlb->entries[replace_index].frame_number = frame_number;
    tlb->entries[replace_index].valid = 1;
    tlb->entries[replace_index].last_used = tlb->current_time++;
    tlb->entries[replace_index].access_count = 1; // 초기화
    tlb->entries[replace_index].read_only = read_only; // 읽기 전용 플래그 설정

    //printf("TLB Updated: Policy=%d, Index=%d, Page=0x%X, Frame=%u\n",tlb_cache, replace_index, page_number, frame_number);
}

int select_victim() {
    int victim_index = -1;

    if (page_replace == 0) { // FIFO
        // FIFO 정책: MemoryMap에서 가장 낮은 arrival_time을 가진 프레임 선택
        uint32_t min_time = UINT32_MAX;
        for (int i = 0; i < frame_count; i++) {
            if (memory_map[i].is_valid && memory_map[i].arrival_time < min_time) {
                min_time = memory_map[i].arrival_time;
                victim_index = i;
            }
        }
    } 
    else if (page_replace == 1) { // LRU
        uint32_t min_time = UINT32_MAX;
        for (int i = 0; i < frame_count; i++) {
            if (memory_map[i].is_valid && memory_map[i].last_used < min_time) {
                min_time = memory_map[i].last_used;
                victim_index = i;
            }
        }
    } 
    else if (page_replace == 2) { // LFU
        uint32_t min_count = UINT32_MAX;
        for (int i = 0; i < frame_count; i++) {
            if (memory_map[i].is_valid && memory_map[i].access_count < min_count) {
                min_count = memory_map[i].access_count;
                victim_index = i;
            }
        }
    } 
    else if (page_replace == 3) { // SECOND_CHANCE
        static int clock_hand = 0; // CLOCK 알고리즘 포인터
        while (1) {
            if (memory_map[clock_hand].is_valid) {
                if (memory_map[clock_hand].reference_bit == 0) {
                    victim_index = clock_hand;
                    clock_hand = (clock_hand + 1) % frame_count;
                    break;
                } else {
                    memory_map[clock_hand].reference_bit = 0; // 참조 비트 초기화
                }
            }
            clock_hand = (clock_hand + 1) % frame_count;
        }
    } else {
        //printf("Invalid page replacement policy.\n");
    }

    return victim_index;
}

int find_disk_block() {
    for (int i = 0; i < MAX_DISK_SIZE/page_size; i++) {
        if (!disk_map[i].is_valid) {
            return i; // 빈 블록의 인덱스를 반환
        }
    }
    return -1; // 빈 블록을 찾을 수 없음
}

void swap_in(FirstPageTable* page_table,int src_disk_block, int dst_memory_frame) {
    if (!disk_map[src_disk_block].is_valid) {
        //printf("Error: Disk block %d is not valid.\n", src_disk_block);
        return;
    }
    increment_swap_in();
    // 디스크에서 메모리로 데이터 복사
    memcpy(&memory[dst_memory_frame * page_size], &disk[src_disk_block * page_size], page_size);

    // 메모리 맵 갱신
    memory_map[dst_memory_frame].is_valid = 1; // 해당 프레임 유효
    memory_map[dst_memory_frame].virtual_address = disk_map[src_disk_block].virtual_address; // 매핑된 VA

    // 정책에 따른 필드 초기화
    if (page_replace == 0) { // FIFO
        memory_map[dst_memory_frame].arrival_time = access_counter; // 현재 접근 카운터로 설정
    } else if (page_replace == 1) { // LRU
        memory_map[dst_memory_frame].last_used = access_counter; // 최근 사용 시간 업데이트
    } else if (page_replace == 2) { // LFU
        memory_map[dst_memory_frame].access_count = 0; // 접근 횟수 초기화
    } else if (page_replace == 3) { // SECOND_CHANCE
        memory_map[dst_memory_frame].reference_bit = 0; // 참조 비트 초기화
    }

    // 디스크 맵 갱신
    disk_map[src_disk_block].is_valid = 0; // 디스크 블록 비우기


    // 페이지 테이블 업데이트
    uint32_t va = disk_map[src_disk_block].virtual_address; // 디스크에서 가져온 VA
    uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
    uint32_t index1 = va >> (offset_bits + second_page);             // index1 추출

    if (page_table->second_level_tables[index1] != NULL) {
        page_table->second_level_tables[index1]->entries[index2].is_valid = 1;  // 페이지 테이블 유효성 활성화
        page_table->second_level_tables[index1]->entries[index2].frame_number = dst_memory_frame; // 메모리 프레임 번호 갱신
    } else {
        //printf("Error: Page table entry for VA=0x%X is NULL.\n", va);
    }

    //printf("Swap In: Disk Block %d -> Memory Frame %d\n", src_disk_block, dst_memory_frame);
}

void init_tlb_entry(TLBEntry* entry) {
    if (entry == NULL) {
        //printf("Error: TLB entry is NULL.\n");
        return;
    }
    
    entry->valid = 0;           // 유효하지 않음
    entry->page_number = 0;     // 페이지 번호 초기화
    entry->frame_number = 0;    // 프레임 번호 초기화
    entry->last_used = 0;       // 마지막 사용 시간 초기화
    entry->access_count = 0;    // 접근 횟수 초기화
    entry->read_only = 0;       // 읽기 전용 플래그 초기화
}


char read_va(FirstPageTable* page_table,uint32_t va) {

    increment_process_count();
    // TLB 접근 시도
    fprintf(write_fp,"=====================================================\n");
    fprintf(write_fp,"\nReading Access : VA=0x%x\n",va);
    if (tlb_size != 0) {
        int tlb_idx = search_tlb(va);
        if (tlb_idx != -1) {
           fprintf(write_fp,"TLB Hit: VA=0x%X, TLB Index=%d\n", va, tlb_idx);
           increment_tlb_hit();

            // TLB 히트 처리
            uint32_t frame_number = tlb->entries[tlb_idx].frame_number;
            uint32_t offset = va & ((1 << offset_bits) - 1);
            uint32_t pa = (frame_number * page_size) + offset; // 물리 주소 계산

           fprintf(write_fp,"TLB Hit: VA=0x%X -> PA=0x%X, Frame=%d\n", va, pa, frame_number);

            // 정책별 메모리 맵 업데이트
            switch (page_replace) {
                case 0: // FIFO
                    break; // FIFO는 읽기 시 별도 업데이트 필요 없음
                case 1: // LRU
                    memory_map[frame_number].last_used = access_counter;
                    break;
                case 2: // LFU
                    memory_map[frame_number].access_count++;
                    break;
                case 3: // SECOND_CHANCE
                    memory_map[frame_number].reference_bit = 1;
                    break;
                default:
                   fprintf(write_fp,"Error: Unknown page replacement policy.\n");
                    break;
            }

           fprintf(write_fp,"Memory Read: PA=0x%X, Value=%c\n\n", pa, memory[pa]);
            return memory[pa]; // 메모리에서 값 읽기
        } else {
            // TLB 미스 처리
           fprintf(write_fp,"TLB Miss: VA=0x%X\n", va);
           increment_tlb_miss();
        }
    }

   fprintf(write_fp,"Accessing Page Table: VA=0x%X\n", va);
    uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
    uint32_t index1 = va >> (offset_bits + second_page); // index1 추출

   fprintf(write_fp,"Page Table Indices: index1=%u, index2=%u\n", index1, index2);

    // 페이지 테이블로 접근
    //printf("%d\n",page_table->second_level_tables[index1]->entries[index2].is_valid);
    if (page_table->second_level_tables[index1]->entries[index2].is_valid) {
        // Page Hit 처리
        increment_page_hit();
        uint32_t offset = va & ((1 << offset_bits) - 1); // 오프셋 추출
        uint32_t frame_number = page_table->second_level_tables[index1]->entries[index2].frame_number;
        uint8_t read_only = page_table->second_level_tables[index1]->entries[index2].read_only;
        uint32_t pa = (frame_number * page_size) + offset; // 물리 주소 계산

       fprintf(write_fp,"Page Table Hit: VA=0x%X -> PA=0x%X, Frame=0x%x\n", va, pa, frame_number);

        // TLB 업데이트
        if (tlb_size > 0) {
            update_tlb(va, frame_number, read_only); // TLB 갱신
           fprintf(write_fp,"TLB Updated: VA=0x%X ReadOnly=%d\n", va, read_only);
        }

        // 정책별 메모리 맵 업데이트
        switch (page_replace) {
            case 0: // FIFO
                break; // FIFO는 읽기 시 별도 업데이트 필요 없음
            case 1: // LRU
                memory_map[frame_number].last_used = access_counter;
                break;
            case 2: // LFU
                memory_map[frame_number].access_count++;
                break;
            case 3: // SECOND_CHANCE
                memory_map[frame_number].reference_bit = 1;
                break;
            default:
               fprintf(write_fp,"Error: Unknown page replacement policy.\n");
                break;
        }

       fprintf(write_fp,"Memory Read: PA=0x%X, Value=%c\n\n", pa, memory[pa]);
        return memory[pa]; // 메모리에서 값 읽기
    } else {
        // Page Fault 처리
        increment_page_fault();
       fprintf(write_fp,"\nPage Fault: VA=0x%X\n", va);

        uint32_t offset = va & ((1 << offset_bits) - 1); // 오프셋 추출
        uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
        uint32_t index1 = va >> (offset_bits + second_page); // index1 추출

        int mem_idx = is_memory_full(); // 메모리 내 빈 자리 확인

        if (mem_idx != -1) {
            uint32_t disk_number = page_table->second_level_tables[index1]->entries[index2].disk_block;
           fprintf(write_fp,"Page Fault Recovery: Loading Disk Block=%d into Frame=%d\n", disk_number, mem_idx);
            swap_in(page_table, disk_number, mem_idx); // 디스크에서 메모리로 페이지 로드
        } else {
            // Victim 선택 후 Swap Out
            mem_idx = select_victim();
            int new_disk_idx = find_disk_block();
            if (new_disk_idx == -1) {
               fprintf(write_fp,"Error: No free disk blocks available.\n");
                return 0; // 읽기 실패
            }
           fprintf(write_fp,"Page Fault Recovery: Evicting Frame=%d to Disk Block=%d\n", mem_idx, new_disk_idx);
            swap_out(page_table,mem_idx, new_disk_idx, 1);
        
            uint32_t disk_number = page_table->second_level_tables[index1]->entries[index2].disk_block;
           fprintf(write_fp,"Page Fault Recovery: Loading Disk Block=%d into Frame=%d\n", disk_number, mem_idx);
            swap_in(page_table, disk_number, mem_idx);
        }

        // Swap 후 메모리 접근
        uint32_t frame_number = page_table->second_level_tables[index1]->entries[index2].frame_number;
        uint32_t pa = (frame_number * page_size) + offset; // 물리 주소 계산

       //fprintf(write_fp,"Page Fault Resolved: VA=0x%X -> PA=0x%X, Frame=%d\n", va, pa, frame_number);

        // TLB 업데이트
        if (page_table->second_level_tables[index1] != NULL &&
            page_table->second_level_tables[index1]->entries[index2].is_valid) {
            uint8_t read_only = page_table->second_level_tables[index1]->entries[index2].read_only;
            update_tlb(va, frame_number, read_only);
        }

       fprintf(write_fp,"Memory Read: PA=0x%X, Value=%c\n\n", pa, memory[pa]);
        return memory[pa]; // 메모리에서 값 읽기
    }
}



void write_va(FirstPageTable* page_table, uint32_t va, char value) {

    increment_process_count();
    // Step 1: TLB 접근
    fprintf(write_fp,"=====================================================\n");
    fprintf(write_fp, "\nWriting Access : VA=0x%X, Value='%c'\n", va, value);
    if (tlb_size != 0) {
        int tlb_idx = search_tlb(va);
        if (tlb_idx != -1) {
            // Step 1.1: TLB Hit
            increment_tlb_hit();
            uint32_t frame_number = tlb->entries[tlb_idx].frame_number;
            uint32_t offset = va & ((1 << offset_bits) - 1);
            uint32_t pa = (frame_number * page_size) + offset; // 물리 주소 계산

            if (tlb->entries[tlb_idx].read_only) {
                // Case: Copy-on-Write 발생
                fprintf(write_fp,"TLB Hit: Copy-on-Write triggered for VA=0x%X\n", va);

                // 새로운 페이지 생성 및 메모리 할당
                int new_frame = is_memory_full();
                if (new_frame == -1) {
                    // 메모리가 꽉 찬 경우 - Victim 페이지 스왑 아웃
                    new_frame = select_victim();
                    int new_disk_idx = find_disk_block();
                    if (new_disk_idx == -1) {
                        printf("Error: No free disk blocks available.\n");
                        return;
                    }
                    swap_out(page_table,new_frame, new_disk_idx,1);
                }

                // 기존 데이터를 새로운 프레임으로 복사
                memcpy(&memory[new_frame * page_size], &memory[frame_number * page_size], page_size);

                // PageTable 업데이트
                uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1);
                uint32_t index1 = va >> (offset_bits + second_page);
                page_table->second_level_tables[index1]->entries[index2].frame_number = new_frame;
                page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;
                page_table->second_level_tables[index1]->entries[index2].read_only = 0;

                // MemoryMap 업데이트
                memory_map[new_frame].virtual_address = va;
                memory_map[new_frame].is_valid = 1;
                memory_map[new_frame].access_count = 0; // 초기화
                memory_map[new_frame].last_used = access_counter; // LRU 정책 반영

                // TLB 업데이트
                update_tlb(va, new_frame, 0);

                // 값 쓰기
                pa = (new_frame * page_size) + offset;
                memory[pa] = value;
            } else {
                // Case: Read-Only 비활성화 (직접 쓰기 가능)
                fprintf(write_fp, "TLB Hit: Write access granted for VA=0x%X\n", va);
                memory[pa] = value;

                // PageTable 업데이트
                uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1);
                uint32_t index1 = va >> (offset_bits + second_page);
                page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;

                // MemoryMap 업데이트
                switch (page_replace) {
                    case 0: // FIFO
                        memory_map[frame_number].arrival_time = access_counter; // 도착 시간 갱신
                        break;

                    case 1: // LRU
                        memory_map[frame_number].last_used = access_counter; // 최근 사용 시간 갱신
                        break;

                    case 2: // LFU
                        memory_map[frame_number].access_count++; // 접근 횟수 증가
                        break;

                    case 3: // SECOND_CHANCE
                        memory_map[frame_number].reference_bit = 1; // 참조 비트 설정
                        break;

                    default:
                        //printf("Error: Unknown page replacement policy.\n");
                        break;
                }

            }
            fprintf(write_fp, "Memory Write: PA=0x%X, Value='%c'\n\n", pa, value);
            return;
        } else {
            fprintf(write_fp, "TLB Miss: VA=0x%X\n", va);
        }
    }

    increment_tlb_miss();
    // Step 2: TLB Miss -> PageTable 접근
    fprintf(write_fp, "Accessing Page Table: VA=0x%X\n", va);
    uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1);
    uint32_t index1 = va >> (offset_bits + second_page);
    fprintf(write_fp, "Page Table Indices: index1=%u, index2=%u\n", index1, index2);

    if (page_table->second_level_tables[index1] != NULL &&
        page_table->second_level_tables[index1]->entries[index2].is_valid) {
        // Step 2.1: PageTable Hit
        increment_page_hit();
        uint32_t frame_number = page_table->second_level_tables[index1]->entries[index2].frame_number;

        if (page_table->second_level_tables[index1]->entries[index2].read_only) {
            // Case: Copy-on-Write 발생
            fprintf(write_fp, "Page Table Hit: Copy-on-Write triggered for VA=0x%X\n", va);

            // 새로운 페이지 생성 및 메모리 할당
            int new_frame = is_memory_full();
            if (new_frame == -1) {
                new_frame = select_victim();
                int new_disk_idx = find_disk_block();
                if (new_disk_idx == -1) {
                    printf("Error: No free disk blocks available.\n");
                    return;
                }
                swap_out(page_table,new_frame, new_disk_idx,1);
            }

            // 기존 데이터를 새로운 프레임으로 복사
            memcpy(&memory[new_frame * page_size], &memory[frame_number * page_size], page_size);

            // PageTable 업데이트
            page_table->second_level_tables[index1]->entries[index2].frame_number = new_frame;
            page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;
            page_table->second_level_tables[index1]->entries[index2].read_only = 0;

            // MemoryMap 업데이트
            memory_map[new_frame].virtual_address = va;
            memory_map[new_frame].is_valid = 1;
            memory_map[new_frame].access_count = 0;
            memory_map[new_frame].last_used = access_counter;

            // TLB 업데이트
            update_tlb(va, new_frame, 0);

            // 값 쓰기
            uint32_t offset = va & ((1 << offset_bits) - 1);
            uint32_t pa = (new_frame * page_size) + offset;
            memory[pa] = value;
            fprintf(write_fp, "Memory Write: PA=0x%X, Value='%c'\n\n", pa, value);
        } else {
            // Case: Read-Only 비활성화 (직접 쓰기 가능)
            //printf("PageTable Hit: Write access granted for VA=0x%X\n", va);
            uint32_t offset = va & ((1 << offset_bits) - 1);
            uint32_t pa = (frame_number * page_size) + offset;
            memory[pa] = value;

            // PageTable 업데이트
            page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;

            // MemoryMap 업데이트
            // MemoryMap 업데이트
            switch (page_replace) {
                case 0: // FIFO
                    memory_map[frame_number].arrival_time = access_counter; // 도착 시간 갱신
                    break;

                case 1: // LRU
                    memory_map[frame_number].last_used = access_counter; // 최근 사용 시간 갱신
                    break;

                case 2: // LFU
                    memory_map[frame_number].access_count++; // 접근 횟수 증가
                    break;

                case 3: // SECOND_CHANCE
                    memory_map[frame_number].reference_bit = 1; // 참조 비트 설정
                    break;

                default:
                    fprintf(write_fp, "Error: Unknown page replacement policy.\n"); break;
                    break;
            }

        }
    } 
    else {
        // Step 2.2: PageTable Miss (Page Fault 발생)
        increment_page_fault();
        fprintf(write_fp, "Page Fault: VA=0x%X\n", va);

        // Index 추출
        uint32_t offset = va & ((1 << offset_bits) - 1); // 오프셋 추출
        uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
        uint32_t index1 = va >> (offset_bits + second_page); // index1 추출

        // 메모리 내 빈 자리 확인
        int mem_idx = is_memory_full();

        // 메모리가 꽉 찬 경우 Victim 교체
        if (mem_idx == -1) {
            mem_idx = select_victim();
            int new_disk_idx = find_disk_block();
            if (new_disk_idx == -1) {
                printf("Error: No free disk blocks available.\n");
                return;
            }
            swap_out(page_table,mem_idx, new_disk_idx,1);
        }

        // Disk에서 Swap-In
        uint32_t disk_block = page_table->second_level_tables[index1]->entries[index2].disk_block;

        if (page_table->second_level_tables[index1]->entries[index2].read_only) {
            // Case: Copy-on-Write 발생
            //printf("Copy-on-Write triggered for VA=0x%X\n", va);

            // 새로운 메모리 프레임 할당
            int new_frame = is_memory_full();
            if (new_frame == -1) {
                new_frame = select_victim();
                int new_disk_idx = find_disk_block();
                if (new_disk_idx == -1) {
                    //printf("Error: No free disk blocks available.\n");
                    return;
                }
                swap_out(page_table,new_frame, new_disk_idx,1);
            }

            // 기존 데이터 복사
            swap_in(page_table,disk_block, new_frame);

            // PageTable 및 MemoryMap 업데이트
            page_table->second_level_tables[index1]->entries[index2].frame_number = new_frame;
            page_table->second_level_tables[index1]->entries[index2].read_only = 0;
            page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;

            memory_map[new_frame].virtual_address = va;
            memory_map[new_frame].is_valid = 1;
            memory_map[new_frame].last_used = access_counter;

            // MemoryMap 정책별 업데이트
            switch (page_replace) {
                case 0: memory_map[new_frame].arrival_time = access_counter; break; // FIFO
                case 1: memory_map[new_frame].last_used = access_counter; break;   // LRU
                case 2: memory_map[new_frame].access_count = 0; break;             // LFU
                case 3: memory_map[new_frame].reference_bit = 0; break;            // SECOND_CHANCE
            }

            // TLB 업데이트
            update_tlb(va, new_frame, 0);

            // 값 쓰기
            uint32_t pa = (new_frame * page_size) + offset;
            memory[pa] = value;

        } else {
            // Case: Read-Only 비활성화 또는 Write 가능한 상태
            swap_in(page_table,disk_block, mem_idx);

            // PageTable 및 MemoryMap 업데이트
            page_table->second_level_tables[index1]->entries[index2].frame_number = mem_idx;
            page_table->second_level_tables[index1]->entries[index2].is_valid = 1;
            page_table->second_level_tables[index1]->entries[index2].is_dirty = 1;

            memory_map[mem_idx].virtual_address = va;
            memory_map[mem_idx].is_valid = 1;
            memory_map[mem_idx].last_used = access_counter;

            // MemoryMap 정책별 업데이트
            switch (page_replace) {
                case 0: memory_map[mem_idx].arrival_time = access_counter; break; // FIFO
                case 1: memory_map[mem_idx].last_used = access_counter; break;   // LRU
                case 2: memory_map[mem_idx].access_count = 0; break;             // LFU
                case 3: memory_map[mem_idx].reference_bit = 0; break;            // SECOND_CHANCE
            }

            // TLB 업데이트
            update_tlb(va, mem_idx, 0);

            // 값 쓰기
            uint32_t pa = (mem_idx * page_size) + offset;
            memory[pa] = value;
            fprintf(write_fp, "Memory Write: PA=0x%X, Value='%c'\n\n", pa, value);
        }
    }

}

void swap_out(FirstPageTable* page_table,int src_memory_frame, int dst_disk_block, int status) {

    increment_swap_out();

    if (memory_map[src_memory_frame].is_valid == 0) {
        fprintf(write_fp, "Error: Memory frame %d is not valid.\n", src_memory_frame);
        return;
    }

    if (dst_disk_block < 0 || dst_disk_block >= MAX_DISK_SIZE) {
        fprintf(write_fp, "Error: Disk block %d is out of range.\n", dst_disk_block);
        return;
    }

    // 메모리에서 디스크로 데이터 복사
    for (uint32_t i = 0; i < page_size; i++) {
        disk[dst_disk_block * page_size + i] = memory[src_memory_frame * page_size + i];
    }

    // 디스크 맵 갱신
    disk_map[dst_disk_block].is_valid = 1;
    disk_map[dst_disk_block].virtual_address = memory_map[src_memory_frame].virtual_address;

    // 페이지 테이블 업데이트
    uint32_t va = memory_map[src_memory_frame].virtual_address; // VA 가져오기
    uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
    uint32_t index1 = va >> (offset_bits + second_page);             // index1 추출

    if (page_table->second_level_tables[index1] != NULL) {
        PageTableEntry* entry = &page_table->second_level_tables[index1]->entries[index2];
        entry->is_valid = 0; 
        entry->disk_block = dst_disk_block; 
        entry->frame_number = 0;

        fprintf(write_fp, "PageTable Update: VA=0x%X, Index1=%u, Index2=%u -> Disk Block=%d\n", 
                va, index1, index2, dst_disk_block);
    } else {
        fprintf(write_fp, "Error: Page table entry for VA=0x%X is NULL. Skipping update.\n", va);
        return;
    }

    // 메모리 맵 갱신
    memory_map[src_memory_frame].is_valid = 0;
    memory_map[src_memory_frame].virtual_address = 0; // VA 초기화

    fprintf(write_fp, "Swap Out: Memory Frame %d -> Disk Block %d, Updated Page Table VA=0x%X\n", 
            src_memory_frame, dst_disk_block, va);
}


void initial_allocate(FirstPageTable* page_table,uint32_t va, char value) {
    // Index 추출
    uint32_t offset = va & ((1 << offset_bits) - 1); // 오프셋 추출
    uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1); // index2 추출
    uint32_t index1 = va >> (offset_bits + second_page); // index1 추출

    // 이미 VA가 할당되었는지 확인
    if (page_table->second_level_tables[index1] != NULL &&
        page_table->second_level_tables[index1]->entries[index2].is_valid) {
        // 이미 할당된 경우 -> 물리적 주소 계산 및 값 변경
        uint32_t frame_number = page_table->second_level_tables[index1]->entries[index2].frame_number;
        uint32_t pa = (frame_number * page_size) + offset; // 물리 주소 계산
        memory[pa] = value; // 메모리에 값 쓰기

        //Debugging 출력
        //fprintf(write_fp,"[Existing Allocation] VA=0x%X -> PA=0x%X, Frame=0x%X, Updated Value='%c'\n",va, pa, frame_number, value);
        return;
    }


    fprintf(write_fp,"\n\n[Existing Allocation] VA=0x%X Updated Value='%c'\n",va, value);
    // 메모리 내 빈 자리 확인
    int mem_idx = is_memory_full();

    // 메모리가 꽉 찬 경우 랜덤으로 Victim 교체
    if (mem_idx == -1) {
        // 랜덤 Victim 선택
        mem_idx = rand() % frame_count; // frame_count: 총 메모리 프레임 개수
        //printf("Random Victim Selected: Frame 0x%X\n", mem_idx);

        // 디스크에서 빈 블록 찾기
        int new_disk_idx = find_disk_block();
        if (new_disk_idx == -1) {
            //printf("Error: No free disk blocks available.\n");
            return;
        }

        fprintf(write_fp,"swap_out : mem=0x%x -> disk=0x%x\n",mem_idx,new_disk_idx);    

        // Victim 페이지 스왑 아웃
        swap_out(page_table,mem_idx, new_disk_idx, 0);
        // swap_out 후 해당 디스크 블록의 상태를 출력
        fprintf(write_fp, "After Swap Out: Disk Block=%d, Stored VA=0x%X\n", 
                new_disk_idx, disk_map[new_disk_idx].virtual_address);

        // 관련 페이지 테이블 상태를 출력
        uint32_t va = disk_map[new_disk_idx].virtual_address;
        uint32_t index2 = (va >> offset_bits) & ((1 << second_page) - 1);
        uint32_t index1 = va >> (offset_bits + second_page);

        if (page_table->second_level_tables[index1] != NULL) {
            PageTableEntry* entry = &page_table->second_level_tables[index1]->entries[index2];
            fprintf(write_fp, "Page Table Entry After Swap Out: VA=0x%X, Index1=%u, Index2=%u  ",va, index1, index2);
            fprintf(write_fp, "  is_valid=%d, disk_block=%d, frame_number=%d\n", 
                    entry->is_valid, entry->disk_block, entry->frame_number);
        } else {
            fprintf(write_fp, "Error: Page table entry for VA=0x%X is NULL.\n", va);
        }
    }

    // MemoryMap 업데이트
    //fprintf(write_fp,"mem_map before : mem_idx=0x%x, va1=0x%x, is_valid=%d\n",mem_idx, memory_map[mem_idx].virtual_address,memory_map[mem_idx].is_valid);
    memory_map[mem_idx].is_valid = 1;           // 프레임 유효화
    memory_map[mem_idx].virtual_address = va;  // VA와 매핑
    //fprintf(write_fp,"mem_map after : mem_idx=0x%x, va1=0x%x, is_valid=%d\n",mem_idx, memory_map[mem_idx].virtual_address,memory_map[mem_idx].is_valid);


   fprintf(write_fp,"[Initial Allocation] Frame=0x%X, VA=0x%X\n", mem_idx, va);
    switch (page_replace) {
        case 0: // FIFO
            memory_map[mem_idx].arrival_time = access_counter;
            break;

        case 1: // LRU
            memory_map[mem_idx].last_used = access_counter;
            break;

        case 2: // LFU
            memory_map[mem_idx].access_count = 0;
            break;

        case 3: // SECOND_CHANCE
            memory_map[mem_idx].reference_bit = 0;
            break;

        default:
           fprintf(write_fp,"Error: Unknown page replacement policy.\n");
            return;
    }

    // PageTable 업데이트
    //fprintf(write_fp, "Index1: %d, Index2: %d\n",index1,index2);

    // 2단계 페이지 테이블 엔트리 업데이트
    page_table->second_level_tables[index1]->entries[index2].is_valid = 1;
    page_table->second_level_tables[index1]->entries[index2].frame_number = mem_idx;
    page_table->second_level_tables[index1]->entries[index2].disk_block = 0;
    page_table->second_level_tables[index1]->entries[index2].read_only = 1; // 읽기 전용 설정
    page_table->second_level_tables[index1]->entries[index2].is_dirty = 0;  // 초기 상태
    
    // 값 쓰기
    uint32_t pa = (mem_idx * page_size) + offset;
    memory[pa] = value;

    // Debug
   fprintf(write_fp,"[Init Write] VA=0x%X -> PA=0x%X, Frame=0x%X, Value='%c'\n", va, pa, mem_idx, value);
}

void dump_page_table(FirstPageTable* page_table) {
    if (!write_fp) {
        fprintf(stderr, "Error: Invalid file pointer for page table dump.\n");
        return;
    }

    fprintf(write_fp, "Dumping Page Table State:\n");
    for (uint32_t i = 0; i < (1 << first_page); i++) { // 첫 번째 레벨 페이지 테이블 크기만큼 반복
        if (page_table->second_level_tables[i] != NULL) {
            fprintf(write_fp, "First Level Index: %u\n", i);
            for (uint32_t j = 0; j < (1 << second_page); j++) { // 두 번째 레벨 페이지 테이블 크기만큼 반복
                PageTableEntry* entry = &page_table->second_level_tables[i]->entries[j];
                if (entry->is_valid || entry->disk_block > 0) { // 유효한 페이지 또는 디스크 블록이 존재하는 경우만 출력
                    fprintf(write_fp,
                            "  Second Level Index: %u, Frame: 0x%X, is_valid: %d, disk_block: %d, is_dirty: %d\n",
                            j, entry->frame_number, entry->is_valid, entry->disk_block, entry->is_dirty);
                }
            }
        }
    }
    fprintf(write_fp, "Page Table Dump Complete.\n");
}



void free_memory_manager() {
    free(disk);
    disk = NULL;

    free(disk_map);
    disk_map = NULL;

    free(memory);
    memory = NULL;

    free(memory_map);
    memory_map = NULL;
}

