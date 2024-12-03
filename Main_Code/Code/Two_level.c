#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 상수 정의
#define PAGE_SIZE 4096                              // 페이지 크기 (4KB)
#define PHYSICAL_MEMORY_SIZE (16 * 1024 * 1024)     // 물리 메모리 크기 (16MB)
#define VIRTUAL_MEMORY_SIZE (4 * 1024 * 1024)       // 가상 메모리 크기 (4MB)
#define NUM_PROCESSES 10                            // 사용자 프로세스 수
#define TICKS 10000                                 // 시뮬레이션 총 tick 수
#define FIRST_LEVEL_ENTRIES 32                      // 1단계 페이지 테이블 엔트리 수
#define SECOND_LEVEL_ENTRIES 128                    // 2단계 페이지 테이블 엔트리 수
#define PAGES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)    // 물리 페이지 수

// 페이지 테이블 엔트리 구조체
typedef struct {
    int frame_number;               // 페이지가 매핑된 프레임 번호
    int valid;                      // 유효 플래그 (1: 유효, 0: 무효)
} page_table_entry;

// 프로세스 구조체
typedef struct {
    page_table_entry *first_level[FIRST_LEVEL_ENTRIES]; // 1단계 페이지 테이블
    int pid;                                            // 프로세스 ID
    int page_faults;                                    // 페이지 폴트 횟수
    int memory_accesses;                                // 메모리 접근 횟수
} process;

// 전역 변수
process processes[NUM_PROCESSES];
int free_page_list[PAGES];          // 사용 가능한 프레임 번호 리스트
int free_page_count = PAGES;
int page_fault_count = 0;

// 로그 파일
FILE *simulation_log;

// 함수 선언
void initialize_system();
int allocate_free_page();
void handle_page_fault(int process_id, int virtual_address, int tick);
void simulate_memory_access(int tick);
void write_summary_to_terminal_and_log(int tick);
void write_logs();

int main() {
    clock_t start_time, end_time;
    double simulation_time;

    // 시스템 초기화
    initialize_system();

    // 로그 파일 열기
    simulation_log = fopen("simulation.txt", "w");
    if (!simulation_log) {
        perror("로그 파일 열기에 실패했습니다.");
        exit(EXIT_FAILURE);
    }

    // 시뮬레이션 실행
    start_time = clock();
    for (int tick = 1; tick <= TICKS; tick++) {
        simulate_memory_access(tick);

        // 1000 tick마다 터미널 및 로그 파일에 요약 출력
        if (tick % 1000 == 0) {
            write_summary_to_terminal_and_log(tick);
        }
    }
    end_time = clock();

    // 시뮬레이션 시간 계산
    simulation_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    // 로그 작성 및 파일 닫기
    write_logs();
    fclose(simulation_log);

    // 최종 결과 출력
    printf("\n시뮬레이션 완료.\n");
    printf("총 시뮬레이션 시간: %.6f초\n", simulation_time);
    printf("전체 페이지 폴트 수: %d\n", page_fault_count);

    return 0;
}

// 시스템 초기화 함수
void initialize_system() {
    for (int i = 0; i < PAGES; i++) {
        free_page_list[i] = i;
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].pid = i + 1;
        processes[i].page_faults = 0;
        processes[i].memory_accesses = 0;
        memset(processes[i].first_level, 0, sizeof(processes[i].first_level)); // 1단계 페이지 테이블 초기화
    }
}

// 사용 가능한 프레임 할당
int allocate_free_page() {
    if (free_page_count > 0) {
        return free_page_list[--free_page_count]; // 사용 가능한 프레임 반환
    } else {
        // 메모리가 부족한 경우 로그만 기록하고 진행
        fprintf(simulation_log, "메모리가 부족합니다. 기존 프레임을 재사용합니다.\n");

        // 재사용할 프레임을 강제로 반환 (예: 0번째 프레임 반환)
        return rand() % PAGES; // 임의의 프레임을 재사용
    }
}

// 페이지 폴트 처리
void handle_page_fault(int process_id, int virtual_address, int tick) {
    process *proc = &processes[process_id];
    int first_level_index = (virtual_address >> 12) & 0x1F;     // 상위 5비트 (1단계 테이블 인덱스)
    int second_level_index = (virtual_address >> 7) & 0x7F;     // 그다음 7비트 (2단계 테이블 인덱스)

    // 1단계 페이지 테이블 확인
    if (!proc->first_level[first_level_index]) {
        proc->first_level[first_level_index] = malloc(sizeof(page_table_entry) * SECOND_LEVEL_ENTRIES);
        memset(proc->first_level[first_level_index], 0, sizeof(page_table_entry) * SECOND_LEVEL_ENTRIES); // 초기화
    }

    page_table_entry *second_level_table = proc->first_level[first_level_index];

    // 2단계 페이지 테이블 확인
    if (!second_level_table[second_level_index].valid) {
        // 페이지 폴트 처리: 새로운 프레임 할당
        int free_page = allocate_free_page();
        second_level_table[second_level_index].frame_number = free_page;
        second_level_table[second_level_index].valid = 1;

        fprintf(simulation_log, "Tick: %d, 프로세스 %d, 페이지 폴트: VA %d -> 프레임 %d\n",
                tick, process_id + 1, virtual_address, free_page);

        proc->page_faults++;
        page_fault_count++;
    }
}

// 메모리 접근 시뮬레이션
void simulate_memory_access(int tick) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process *proc = &processes[i];
        for (int j = 0; j < 10; j++) { // 각 프로세스는 tick마다 10번의 메모리 접근 수행
            int virtual_address = rand() % VIRTUAL_MEMORY_SIZE;

            proc->memory_accesses++;

            // 1단계 및 2단계 페이지 테이블 확인
            int first_level_index = (virtual_address >> 12) & 0x1F;     // 상위 5비트 (1단계 테이블 인덱스)
            int second_level_index = (virtual_address >> 7) & 0x7F;     // 그다음 7비트 (2단계 테이블 인덱스)

            page_table_entry *second_level_table = proc->first_level[first_level_index];

            if (!second_level_table || !second_level_table[second_level_index].valid) {
                // 페이지 폴트 처리
                handle_page_fault(i, virtual_address, tick);
            } else {
                // 유효한 페이지 접근
                fprintf(simulation_log, "Tick: %d, 프로세스 %d, VA %d 접근 (프레임 %d)\n",
                        tick, proc->pid, virtual_address, second_level_table[second_level_index].frame_number);
            }
        }
    }
}

// 1000 tick마다 터미널 및 로그 파일에 요약 출력
void write_summary_to_terminal_and_log(int tick) {
    printf("\n=== Tick %d 시뮬레이션 요약 ===\n", tick);
    fprintf(simulation_log, "\n=== Tick %d 시뮬레이션 요약 ===\n", tick);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process *proc = &processes[i];
        printf("프로세스 %d: 페이지 폴트: %d, 메모리 접근: %d\n",
               proc->pid, proc->page_faults, proc->memory_accesses);
        fprintf(simulation_log, "프로세스 %d: 페이지 폴트: %d, 메모리 접근: %d\n",
                proc->pid, proc->page_faults, proc->memory_accesses);
    }
    printf("전체 페이지 폴트 수: %d\n", page_fault_count);
    fprintf(simulation_log, "전체 페이지 폴트 수: %d\n", page_fault_count);
}

// 로그 작성
void write_logs() {
    fprintf(simulation_log, "\n=== 시뮬레이션 최종 요약 ===\n");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        process *proc = &processes[i];
        fprintf(simulation_log, "프로세스 %d: 페이지 폴트: %d, 메모리 접근: %d\n",
                proc->pid, proc->page_faults, proc->memory_accesses);
    }
    fprintf(simulation_log, "전체 페이지 폴트 수: %d\n", page_fault_count);
}