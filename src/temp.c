//
// Created by hochacha on 11/25/24.
//
// 프레임 관리자 초기화
void init_frame_manager() {
    memset(&frame_manager, 0, sizeof(frame_manager_t));
    frame_manager.free_frames = FRAME_COUNT;
}

// 빈 프레임 찾기
int allocate_frame() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (!(frame_manager.bitmap[byte_index] & (1 << bit_index))) {
            // 프레임 할당 표시
            frame_manager.bitmap[byte_index] |= (1 << bit_index);
            frame_manager.free_frames--;
            return i;
        }
    }
    return -1;  // 가용 프레임 없음
}

// 프레임 해제
void free_frame(int frame_number) {
    int byte_index = frame_number / 8;
    int bit_index = frame_number % 8;
    frame_manager.bitmap[byte_index] &= ~(1 << bit_index);
    frame_manager.free_frames++;
}

// 2단계 페이지 테이블 생성
page_table_entry_t* create_page_table() {
    int frame = allocate_frame();
    if (frame == -1) return NULL;

    page_table_entry_t* table = (page_table_entry_t*)malloc(PAGE_SIZE);
    memset(table, 0, PAGE_SIZE);
    return table;
}

// 페이지 테이블 시스템 초기화
int init_paging() {
    init_frame_manager();

    // 1단계 페이지 테이블(디렉토리) 생성
    page_directory = create_page_table();
    if (!page_directory) return -1;

    return 0;
}

// 가상 주소를 물리 주소로 변환
unsigned int virtual_to_physical(unsigned int virtual_addr) {
    virtual_address_t addr = { .value = virtual_addr };

    // 1단계 테이블 접근
    page_table_entry_t* first_entry = &page_directory[addr.parts.first_table];
    if (!first_entry->present) {
        // 2단계 테이블이 없으면 생성
        page_table_entry_t* second_table = create_page_table();
        if (!second_table) return (unsigned int)-1;

        first_entry->present = 1;
        first_entry->writable = 1;
        first_entry->frame = allocate_frame();
    }

    // 2단계 테이블 접근
    page_table_entry_t* second_table = (page_table_entry_t*)(first_entry->frame * PAGE_SIZE);
    page_table_entry_t* second_entry = &second_table[addr.parts.second_table];

    if (!second_entry->present) {
        // 실제 페이지가 없으면 새 프레임 할당
        int frame = allocate_frame();
        if (frame == -1) return (unsigned int)-1;

        second_entry->present = 1;
        second_entry->writable = 1;
        second_entry->frame = frame;
    }

    // 최종 물리 주소 계산
    return (second_entry->frame * PAGE_SIZE) + addr.parts.offset;
}

// 페이지 테이블 정보 출력
void print_page_info(unsigned int virtual_addr) {
    virtual_address_t addr = { .value = virtual_addr };
    printf("Virtual Address: 0x%08x\n", virtual_addr);
    printf("First Table Index: %d\n", addr.parts.first_table);
    printf("Second Table Index: %d\n", addr.parts.second_table);
    printf("Offset: 0x%03x\n", addr.parts.offset);

    unsigned int physical_addr = virtual_to_physical(virtual_addr);
    if (physical_addr != (unsigned int)-1) {
        printf("Physical Address: 0x%08x\n", physical_addr);
    } else {
        printf("Address translation failed\n");
    }
}

// 메모리 정리
void cleanup_paging() {
    if (page_directory) {
        // 모든 2단계 테이블 해제
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            if (page_directory[i].present) {
                page_table_entry_t* second_table =
                    (page_table_entry_t*)(page_directory[i].frame * PAGE_SIZE);
                free(second_table);
                free_frame(page_directory[i].frame);
            }
        }
        free(page_directory);
    }
}

// 테스트를 위한 메인 함수
int main() {
    if (init_paging() < 0) {
        printf("Failed to initialize paging\n");
        return 1;
    }

    // 테스트 가상 주소
    unsigned int test_addr = 0x12345678;
    print_page_info(test_addr);

    // 다른 가상 주소 테스트
    test_addr = 0x87654321;
    print_page_info(test_addr);

    cleanup_paging();
    return 0;
}