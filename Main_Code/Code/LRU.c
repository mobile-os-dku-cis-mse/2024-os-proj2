#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 상수 정의
#define PAGE_SIZE 4096 // 페이지 크기: 4KB
#define PHYSICAL_MEMORY_SIZE (16 * 1024 * 1024) // 물리 메모리 크기: 16MB
#define VIRTUAL_MEMORY_SIZE (4 * 1024 * 1024)   // 가상 메모리 크기: 4MB
#define NUM_PROCESSES 10          // 사용자 프로세스 수
#define TICKS 10000               // 시뮬레이션 총 tick 수
#define PAGES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE) // 물리 메모리 페이지 수
#define VM_PAGES (VIRTUAL_MEMORY_SIZE / PAGE_SIZE) // 가상 메모리 페이지 수
#define PAGE_DIRECTORY_ENTRIES 16 // 2단계 페이지 디렉터리 크기
#define PAGE_TABLE_ENTRIES 256    // 2단계 페이지 테이블 크기

// 페이지 테이블 엔트리 구조체
typedef struct {
    int frame_number;  // 페이지가 매핑된 프레임 번호
    int valid;         // 유효 플래그 (1: 유효, 0: 무효)
    int modified;      // 수정 플래그 (1: 수정됨, 0: 수정 안 됨)
    int referenced;    // 참조 플래그 (1: 참조됨, 0: 참조 안 됨)
} page_table_entry;

// 프로세스 구조체
typedef struct {
    page_table_entry *page_directory[PAGE_DIRECTORY_ENTRIES]; // 2단계 페이지 디렉터리
    int pid;             // 프로세스 ID
    int page_faults;     // 페이지 폴트 횟수
    int memory_accesses; // 메모리 접근 횟수
    int swapped_out;     // 스와핑 횟수
} process;

// 전역 변수
process processes[NUM_PROCESSES];  // 모든 프로세스 정보
int free_page_list[PAGES];         // 사용 가능한 프레임 번호 리스트
int free_page_count = PAGES;       // 사용 가능한 프레임 수
int swap_count = 0;                // 스왑 아웃 횟수
int swap_in_count = 0;             // 스왑 인 횟수
int disk_io_count = 0;             // 디스크 I/O 횟수
int last_used[PAGES];              // 각 프레임의 마지막 사용 tick 기록

// 로그 파일 포인터
FILE *simulation_log;
FILE *swapping_log;

// 함수 선언
void initialize_system();
int allocate_free_page();
void handle_page_fault(int process_id, int virtual_address, int tick, int is_write);
void simulate_memory_access(int tick);
void print_simulation_summary(int tick);
void write_logs();
void cleanup_memory();
int calculate_physical_address(int frame_number, int offset);

int main() {
    clock_t start_time, end_time; // 시뮬레이션 시간 측정
    double simulation_time;

    // 시스템 초기화
    initialize_system();

    // 로그 파일 열기
    simulation_log = fopen("simulation.txt", "w");
    swapping_log = fopen("swapping.txt", "w");

    if (!simulation_log || !swapping_log) {
        perror("로그 파일을 열 수 없습니다.");
        exit(EXIT_FAILURE);
    }

    // 시뮬레이션 시작
    start_time = clock();
    for (int tick = 1; tick <= TICKS; tick++) {
        simulate_memory_access(tick);

        // 1000 tick마다 시뮬레이션 요약 출력
        if (tick % 1000 == 0) {
            print_simulation_summary(tick);
        }
    }
    end_time = clock();

    // 시뮬레이션 소요 시간 계산
    simulation_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    // 로그 작성 및 파일 닫기
    write_logs();
    fclose(simulation_log);
    fclose(swapping_log);

    // 메모리 정리
    cleanup_memory();

    // 최종 요약 출력
    printf("\n시뮬레이션 완료.\n");
    printf("총 시뮬레이션 시간: %.2f초\n", simulation_time);
    printf("전체 페이지 폴트 수: %d\n", swap_count);
    printf("전체 디스크 I/O 횟수: %d\n", disk_io_count);

    return 0;
}

// 시스템 초기화 함수
void initialize_system() {
    for (int i = 0; i < PAGES; i++) {
        free_page_list[i] = i;
        last_used[i] = -1;
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].pid = i + 1;
        processes[i].page_faults = 0;
        processes[i].memory_accesses = 0;
        processes[i].swapped_out = 0;
        memset(processes[i].page_directory, 0, sizeof(processes[i].page_directory));
    }
}

// 사용 가능한 프레임 할당 함수
int allocate_free_page() {
    if (free_page_count > 0) {
        return free_page_list[--free_page_count];
    } else {
        return -1; // 사용 가능한 프레임 없음
    }
}

// 페이지 폴트 처리 함수
void handle_page_fault(int process_id, int virtual_address, int tick, int is_write) {
    int dir_index = (virtual_address >> 20) & 0xF;                      // 상위 4비트 (디렉터리 인덱스)
    int table_index = (virtual_address >> 12) & 0xFF;                   // 중간 8비트 (테이블 인덱스)
    int offset = virtual_address & 0xFFF;                               // 하위 12비트 (오프셋)

    process *proc = &processes[process_id];
    page_table_entry *table = proc->page_directory[dir_index];

    if (!table) {   // 페이지 테이블 생성
        table = malloc(PAGE_TABLE_ENTRIES * sizeof(page_table_entry));
        memset(table, 0, PAGE_TABLE_ENTRIES * sizeof(page_table_entry));
        proc->page_directory[dir_index] = table;
    }

    if (!table[table_index].valid) {
        // 페이지 폴트 처리
        int free_page = allocate_free_page();

        if (free_page == -1) {
            // LRU 알고리즘: 가장 오래 사용되지 않은 프레임 선택
            int lru_page = -1, lru_tick = TICKS;
            int lru_process_id = -1, lru_dir_index = -1, lru_table_index = -1;

            for (int i = 0; i < NUM_PROCESSES; i++) {
                for (int d = 0; d < PAGE_DIRECTORY_ENTRIES; d++) {
                    page_table_entry *dir = processes[i].page_directory[d];
                    if (dir) {
                        for (int t = 0; t < PAGE_TABLE_ENTRIES; t++) {
                            if (dir[t].valid) {
                                int frame = dir[t].frame_number;
                                if (last_used[frame] < lru_tick) {
                                    lru_tick = last_used[frame];
                                    lru_page = frame;
                                    lru_process_id = i;
                                    lru_dir_index = d;
                                    lru_table_index = t;
                                }
                            }
                        }
                    }
                }
            }

            if (lru_page != -1) {
                fprintf(swapping_log, "Tick: %d, 프로세스 %d의 페이지 [%d, %d] (VA: 0x%08X, Frame: %d) 스왑아웃\n",
                        tick, lru_process_id + 1, lru_dir_index, lru_table_index,
                        (lru_dir_index << 20) | (lru_table_index << 12), lru_page);

                page_table_entry *lru_table = processes[lru_process_id].page_directory[lru_dir_index];
                lru_table[lru_table_index].valid = 0;
                free_page = lru_page;
                processes[lru_process_id].swapped_out++;
                swap_count++;
                disk_io_count++;
            } else {
                fprintf(simulation_log, "스왑 아웃 대상이 없습니다. 에러 발생 가능\n");
                exit(EXIT_FAILURE);
            }
        }

        table[table_index].frame_number = free_page;
        table[table_index].valid = 1;
        table[table_index].referenced = 1;
        if (is_write) {
            table[table_index].modified = 1;
        }
        last_used[free_page] = tick;
        proc->page_faults++;
        swap_in_count++;

        // 페이지 테이블 업데이트 로그 작성
        fprintf(simulation_log, "Tick: %d, 프로세스 %d, PAGE TABLE UPDATED: VA 0x%08X -> Frame %d (PA: 0x%08X)\n",
                tick, process_id + 1, virtual_address, free_page,
                calculate_physical_address(free_page, 0));

        fprintf(swapping_log, "Tick: %d, 프로세스 %d의 페이지 [%d, %d] (VA: 0x%08X, Frame: %d) 스왑인\n",
                tick, process_id + 1, dir_index, table_index, virtual_address, free_page);
    }

    fprintf(simulation_log, "Tick: %d, 프로세스 %d, VA: 0x%08X, PA: 0x%08X\n",
            tick, process_id + 1, virtual_address,
            calculate_physical_address(table[table_index].frame_number, offset));
}



// 물리 주소 계산 함수
int calculate_physical_address(int frame_number, int offset) {
    if (frame_number < 0 || offset >= PAGE_SIZE) {
        fprintf(simulation_log, "물리 주소 계산 오류: frame_number=%d, offset=%d\n", frame_number, offset);
        return -1;
    }
    return (frame_number * PAGE_SIZE) + offset;
}


// 메모리 접근 시뮬레이션 함수
void simulate_memory_access(int tick) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process *proc = &processes[i];
        for (int j = 0; j < 10; j++) {
            int virtual_address = rand() % VIRTUAL_MEMORY_SIZE;
            int is_write = rand() % 2;
            proc->memory_accesses++;

            // 페이지 폴트 처리
            handle_page_fault(i, virtual_address, tick, is_write);

            // 읽기/쓰기 이벤트 로그 추가
            fprintf(simulation_log, "Tick: %d, 프로세스 %d, VA: 0x%08X, Access Type: %s\n",
                    tick, proc->pid, virtual_address, is_write ? "Write" : "Read");
        }
    }
}



void fork_process(int parent_pid, int child_pid) {
    process *parent = &processes[parent_pid];
    process *child = &processes[child_pid];

    child->pid = child_pid + 1;
    child->page_faults = 0;
    child->memory_accesses = 0;
    child->swapped_out = 0;

    for (int d = 0; d < PAGE_DIRECTORY_ENTRIES; d++) {
        if (parent->page_directory[d]) {
            child->page_directory[d] = malloc(PAGE_TABLE_ENTRIES * sizeof(page_table_entry));
            for (int t = 0; t < PAGE_TABLE_ENTRIES; t++) {
                child->page_directory[d][t] = parent->page_directory[d][t];
                if (child->page_directory[d][t].valid) {
                    child->page_directory[d][t].referenced = 0; // 자식 초기화
                }
            }
        }
    }

    fprintf(simulation_log, "프로세스 %d가 프로세스 %d를 포크했습니다.\n", parent->pid, child->pid);
}

// 메모리 정리
void cleanup_memory() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int d = 0; d < PAGE_DIRECTORY_ENTRIES; d++) {
            if (processes[i].page_directory[d]) {
                free(processes[i].page_directory[d]);
                processes[i].page_directory[d] = NULL;
            }
        }
    }
}

// 시뮬레이션 요약 출력
void print_simulation_summary(int tick) {
    printf("\n=== Tick %d 시뮬레이션 요약 ===\n", tick);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process *proc = &processes[i];
        printf("프로세스 %d: 페이지 폴트: %d, 메모리 접근: %d, 스왑 아웃: %d, 스왑 아웃: %d\n",
               proc->pid, proc->page_faults, proc->memory_accesses, proc->swapped_out, swap_in_count);
    }
    printf("전체 스왑 아웃 횟수: %d\n", swap_count);
    printf("전체 스왑 인 횟수: %d\n", swap_in_count);
    printf("전체 디스크 I/O 횟수: %d\n", disk_io_count);
}

// 로그 작성
void write_logs() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        fprintf(simulation_log, "프로세스 %d: 페이지 폴트: %d, 메모리 접근: %d, 스왑 아웃: %d\n",
                processes[i].pid, processes[i].page_faults, processes[i].memory_accesses, processes[i].swapped_out);
    }
}