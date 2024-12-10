#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>





typedef struct PageConfig{
    uint32_t offset_bits;
    uint32_t first_page;
    uint32_t second_page;
} PageConfig;


// 페이지 테이블 엔트리 구조체
typedef struct PageTableEntry {
    uint32_t frame_number;  // 물리 프레임 번호
    uint8_t is_valid;       // 유효 비트
    uint8_t is_dirty;       // 수정 비트
    uint32_t disk_block;    // 디스크 블록 번호 (디스크에 있을 때 유효)
    uint8_t read_only;   // 읽기 전용 여부
} PageTableEntry;

// 2단계 페이지 테이블 구조체
typedef struct SecondPageTable {
    PageTableEntry* entries; // 동적으로 할당된 엔트리 배열
    uint32_t size;           // 2단계 테이블의 크기
} SecondPageTable;

// 1단계 페이지 테이블 구조체
typedef struct FirstPageTable {
    SecondPageTable** second_level_tables; // 동적으로 할당된 2단계 테이블 포인터 배열
    uint32_t size;                         // 1단계 테이블의 크기
} FirstPageTable;


FirstPageTable* init_page_table(PageConfig* pc ,int page_size_kb);
void free_page_table(FirstPageTable* page_table);
void copy_page_table(FirstPageTable* src, FirstPageTable* dst);


#endif