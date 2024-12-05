#ifndef VMM2_H
#define VMM2_H

#include <stdint.h>

//1차 설계시 문제점 발생 : 32비트 주소공간 대비 너무 작은 물리메모리(64kb)로 인해 거의 100%확률로 페이지 폴트 발생
//따라서 2차 설계로 물리 메모리 크기를 16MB로 변경 -->  메모리크기 65MB가 적당한 듯 --> 최종 44MB로 변경
//실행 결과 페이지 테이블에 사용되는 프레임의 개수가 1024*10의 10240번까지 인데 페이지 디렉토리 프레임은 포함시키지 않아서 그럼
//만약 페이지 디렉토리 프레임까지 포함시킬경우 1프레임씩 더 사용하여 10250번 프레임까지 페이지 테이블과 디렉토리로 사용되어야 함 ==> 따라서 프레임 개수는 10250보다 크되 스와핑 현상이 일어날 수 있게 너무 크지 않아야 함

//==> 최종적으로 10240 프레임 만큼을 페이지 테이블 10개를 위해 사용하고 1024프레임을 데이터 저장용으로 사용하기로 결정 ==> 최종적으로 10600 또는 10700이 가장 무난

// System Parameters
#define PAGE_SIZE 4096                    // 4KB
#define FRAME_SIZE 4096                   // 4KB
#define NUM_FRAMES 10600//10900//11264//16384//10300//10400//10752//11264//16384  // 페이지 테이블 10개에 사용될 10240개의 프레임 보다는 커야 한다.
#define MAX_VIRT_PAGES 1048576           // 4GB / 4KB
#define SWAP_SIZE (PAGE_SIZE * 1024)      // 4MB swap space

// Page Table Constants
#define PAGE_DIR_SIZE 1024               // 10 bits
#define PAGE_TABLE_SIZE 1024             // 10 bits
#define OFFSET_BITS 12                   // 12 bits for 4KB pages

// Two-Level Page Table Entry
typedef struct {
    uint32_t frame_number:14;     // Physical frame number 
    uint32_t present:1;          // Present in memory == valid bit
    uint32_t dirty:1;           // Modified
    uint32_t accessed:1;        // Recently accessed (for LRU)
    uint32_t user:1;           // User/supervisor mode
    uint32_t writable:1;       // Read/write permissions
    uint32_t reserved:13;      // Reserved bits
} page_table_entry;

// Page Directory Entry
typedef struct {
    uint32_t pt_frame:4;        // Frame containing page table
    uint32_t present:1;         // Page table present in memory
    uint32_t user:1;           // User/supervisor mode
    uint32_t writable:1;       // Read/write permissions
    uint32_t reserved:25;      // Reserved bits
} page_dir_entry;

// Two-Level Page Table
typedef struct {
    page_dir_entry directory[PAGE_DIR_SIZE];
    page_table_entry *tables[PAGE_DIR_SIZE]; 
} PageDirectory;

// Swap Space Entry
typedef struct {
    int process_id;
    uint32_t page_number;
    char data[PAGE_SIZE];
    int is_used;
} SwapEntry;

// Modified MMU for 32-bit
typedef struct {
    PageDirectory *current_pd;    // Current page directory
    struct {
        uint32_t physical_addr;
        int valid;
        char error_msg[100];
    } translation_result;
} MMU32;

// Enhanced Frame Info
typedef struct {
    int is_used;
    int process_id;
    uint32_t virtual_page;
    uint64_t last_access_time;    // For LRU
    int is_page_table;           // Is this frame used for page table 여부판단
} FrameInfo;

typedef enum {
    ALGORITHM_FIFO = 0,
    ALGORITHM_LRU = 1
} ReplacementAlgorithm;

// Global state declarations
extern MMU32 mmu32;
extern FrameInfo frames[NUM_FRAMES];
extern SwapEntry *swap_space;
extern uint64_t global_clock;
extern ReplacementAlgorithm current_algorithm;

// Function declarations
void init_paging_system(void);
uint32_t translate_address(uint32_t virtual_addr);
int handle_page_fault(int process_id, uint32_t virtual_addr);
int swap_out_page(void);
int swap_in_page(int process_id, uint32_t page_number);
void update_lru(int frame_number);
void print_memory_status(void);
int get_free_frame(void);
int find_lru_victim(void);
int find_in_swap(int process_id, uint32_t page_number);
void *get_frame_address(int frame_number);
void print_vm_stats(void);
void update_memory_access(void);
void update_page_fault(void);
void update_page_replacement(void);
void update_swap_stats(int is_swap_in);
int find_free_swap_slot(void);

#endif