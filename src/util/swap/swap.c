//
// Created by hochacha on 24. 12. 2.
//

#include "swap.h"
#include <stdlib.h>
#include <stdint.h> 
#include "../paging/paging.h"

swap_space_t swap_space;

int init_swap_space() {
    swap_space.swap_file = fopen(SWAP_FILE_NAME, "w+b");
    if (swap_space.swap_file == NULL) {
        perror("Failed to create swap file");
        return -1;
    }
    // init swap bitmap
    for (int i = 0; i < MAX_SWAP_PAGES; i++) {
        swap_space.swap_bitmap[i] = 0;
    }
    return 0;
}

int allocate_swap_slot() {
    for (int i = 0; i < MAX_SWAP_PAGES; i++) {
        if (swap_space.swap_bitmap[i] == 0) {
            swap_space.swap_bitmap[i] = 1;
            return i;
        }
    }
    // Exception: Swap space is full
    return -1;
}

void free_swap_slot(int slot) {
    if (slot >= 0 && slot < MAX_SWAP_PAGES) {
        swap_space.swap_bitmap[slot] = 0;
    }
}

// 페이지를 스왑 아웃하는 함수
int swap_out_page(u_pt_t* u_pt, uint16_t vaddr) {
  int slot = allocate_swap_slot();
  if(slot == -1){ 
    fprintf(stderr, "Swap space is full\n");
    return -1; 
  }

  uint32_t swap_offset = slot * PAGE_SIZE; 

  if(fseek(swap_space.swap_file, swap_offset, SEEK_SET) != 0){
    perror("Failed to seek swap file");
    return -1;
  }

  uint8_t *page_data = &physical_mem[]

  // 스왑 아웃할 페이지의 l_pte_t를 가져옵니다.
  uint8_t upper_index = vaddr >> 12;
  uint8_t lower_index = (vaddr >> 8) & 0xF;
  l_pte_t* l_pte = &u_pt->entries[upper_index].lower_table->entries[lower_index];

  // 스왑 파일에서 사용 가능한 오프셋을 찾습니다.
  uint32_t swap_offset = allocate_swap_space();

  // 물리 메모리의 페이지를 스왑 파일에 저장합니다.
  write_page_to_swap(swap_offset, l_pte->frame_number);

  // 페이지 테이블을 갱신합니다.
  l_pte->present = 0;
  l_pte->swapped = 1;
  l_pte->swap_offset = swap_offset;

  // 물리 프레임을 해제합니다.
  free_frame(l_pte->frame_number);

  return 0;
}

int swap_in_page(u_pt_t* u_pt, uint16_t vaddr) {

  uint8_t upper_index = vaddr >> 12;
  uint8_t lower_index = (vaddr >> 8) & 0xF;
  l_pte_t* l_pte = &u_pt->entries[upper_index].lower_table->entries[lower_index];

  // 빈 프레임을 할당
  uint32_t frame_number = allocate_frame();
  if (frame_number == -1) {
    // 페이지 교체 알고리즘을 사용하여 페이지를 스왑 아웃합니다.
    swap_out_page(u_pt, select_victim_page());
    frame_number = allocate_frame();
  }

  // 스왑 파일에서 페이지를 읽어옵니다.
  read_page_from_swap(l_pte->swap_offset, frame_number);

  // 페이지 테이블을 갱신합니다.
  l_pte->frame_number = frame_number;
  l_pte->present = 1;
  l_pte->swapped = 0;

  return 0;
}