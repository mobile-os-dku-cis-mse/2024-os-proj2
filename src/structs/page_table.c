#include "page_table.h"

FirstPageTable* init_page_table(PageConfig* pc ,int page_size_kb) {
    // 페이지 크기에 따른 비트 계산 변수
    FirstPageTable* page_table = (FirstPageTable*)malloc(sizeof(FirstPageTable));
    pc->offset_bits = 0, pc->first_page = 0, pc->second_page = 0;

    // 페이지 크기에 따른 비트 계산
    switch (page_size_kb) {
        case 1*1024: // 1KB 페이지 크기 (10/11/11)
            pc->offset_bits = 10; 
            pc->first_page = 11; 
            pc->second_page = 11;
            break;
        case 4*1024: // 4KB 페이지 크기 (10/11/11)
            pc->offset_bits = 12; 
            pc->first_page = 10; 
            pc->second_page = 10;
            break;
        case 16*1024: // 16KB 페이지 크기 (11/11/10)
            pc->offset_bits = 14; 
            pc->first_page = 9; 
            pc->second_page = 9;
            break;
        case 64*1024: // 64KB 페이지 크기 (14/9/9)
            pc->offset_bits = 16; 
            pc->first_page = 8; 
            pc->second_page = 8;
            break;
        default:
            fprintf(stderr, "Unsupported page size: %d B\n", page_size_kb);
            exit(EXIT_FAILURE);
    }

    // 1단계 및 2단계 테이블 크기 계산
    uint32_t first_level_size = 1 << pc->first_page;  // 2^first_level_bits
    uint32_t second_level_size = 1 << pc->second_page; // 2^second_level_bits

    // 1단계 테이블 초기화
    page_table->size = first_level_size;
    page_table->second_level_tables = (SecondPageTable**)malloc(sizeof(SecondPageTable*) * first_level_size);
    if (page_table->second_level_tables == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for first-level page table.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < first_level_size; i++) {
        page_table->second_level_tables[i] = NULL; // 초기에는 NULL
    }

    // 2단계 테이블 초기화
    for (uint32_t i = 0; i < first_level_size; i++) {
        page_table->second_level_tables[i] = (SecondPageTable*)malloc(sizeof(SecondPageTable));
        if (page_table->second_level_tables[i] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for second-level page table %u.\n", i);
            exit(EXIT_FAILURE);
        }

        SecondPageTable* second_table = page_table->second_level_tables[i];
        second_table->size = second_level_size;
        second_table->entries = (PageTableEntry*)malloc(sizeof(PageTableEntry) * second_level_size);
        if (second_table->entries == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for second-level entries of table %u.\n", i);
            exit(EXIT_FAILURE);
        }

        // 2단계 테이블의 엔트리 초기화
        for (uint32_t j = 0; j < second_level_size; j++) {
            second_table->entries[j].frame_number = 0;
            second_table->entries[j].is_valid = 0;
            second_table->entries[j].is_dirty = 0;
            second_table->entries[j].disk_block = 0;
            second_table->entries[j].read_only = 0;
        }
    }

    printf("Page table initialized with page size: %d B\n", page_size_kb);
    return page_table;
}

void free_page_table(FirstPageTable* page_table) {
    // 2단계 테이블 해제
    for (uint32_t i = 0; i < page_table->size; i++) {
        if (page_table->second_level_tables[i] != NULL) {
            SecondPageTable* second_table = page_table->second_level_tables[i];
            if (second_table->entries != NULL) {
                free(second_table->entries); // 페이지 엔트리 해제
            }
            free(second_table); // 2단계 테이블 해제
        }
    }

    // 1단계 테이블 해제
    if (page_table->second_level_tables != NULL) {
        free(page_table->second_level_tables);
    }

    // 초기화된 구조체 재설정
    page_table->second_level_tables = NULL;
    page_table->size = 0;

    printf("Page table freed.\n");
}


// FirstPageTable 복사 함수
void copy_page_table(FirstPageTable* src, FirstPageTable* dst) {

    
    // 1. dst 초기화
    dst->size = src->size;
    dst->second_level_tables = (SecondPageTable**)malloc(sizeof(SecondPageTable*) * dst->size);
    if (!dst->second_level_tables) {
        fprintf(stderr, "Error: Memory allocation failed for second-level tables in copy_page_table.\n");
        exit(EXIT_FAILURE);
    }

    // 2. 각 2단계 테이블 복사
    for (uint32_t i = 0; i < src->size; i++) {
        if (src->second_level_tables[i]) {
            // 2단계 테이블 할당
            dst->second_level_tables[i] = (SecondPageTable*)malloc(sizeof(SecondPageTable));
            if (!dst->second_level_tables[i]) {
                fprintf(stderr, "Error: Memory allocation failed for second-level table %u in copy_page_table.\n", i);
                exit(EXIT_FAILURE);
            }

            // 2단계 테이블 크기 복사
            dst->second_level_tables[i]->size = src->second_level_tables[i]->size;

            // 2단계 테이블 엔트리 복사
            dst->second_level_tables[i]->entries = (PageTableEntry*)malloc(sizeof(PageTableEntry) * dst->second_level_tables[i]->size);
            if (!dst->second_level_tables[i]->entries) {
                fprintf(stderr, "Error: Memory allocation failed for entries in second-level table %u in copy_page_table.\n", i);
                exit(EXIT_FAILURE);
            }

            for (uint32_t j = 0; j < dst->second_level_tables[i]->size; j++) {
                dst->second_level_tables[i]->entries[j].disk_block = src->second_level_tables[i]->entries[j].disk_block;
                dst->second_level_tables[i]->entries[j].is_valid = src->second_level_tables[i]->entries[j].is_valid;
                dst->second_level_tables[i]->entries[j].frame_number = src->second_level_tables[i]->entries[j].frame_number;
                dst->second_level_tables[i]->entries[j].read_only = 0;
            }
        } else {
            // 2단계 테이블이 없는 경우 NULL로 설정
            dst->second_level_tables[i] = NULL;
        }
    }
}