//
// Created by hochacha on 24. 11. 30.
//

#include "paging.h"
#include "../tlb/tlb.h"
#include <stdlib.h>
#include <stdio.h>
#include "../frame_list/frame_list.h"
#include "../swap/swap.h"
#include "../pid_queue/pid_queue.h"
/*
    System Architecture

    Virttual Memory 
    - 16bit address space
      - 4bit upper index
      - 4bit lower index
      - 8bit offset
    - 256B page size
    - 256 page entries

    Physical Memory
    - 32bit address space
    - 16KB sized shared memory
    - 256B page size

    Swapping
    - 1MB File (let the file stores 4096 pages)
    - 256B page size

*/

extern pcb_t* global_pcb_ptrs[10];

u_pt_t* create_u_pt(void){
  u_pt_t* u_pt = (u_pt_t*)malloc(sizeof(u_pt_t));
  if (u_pt == NULL) {
    perror("Failed to allocate memory for u_pt");
    return NULL;
  }

  for(int i = 0; i< NUM_UPPER_ENTRIES; i++){
    u_pt->entries[i].lower_table = NULL;
    u_pt->entries[i].present = 0;
  }
  return u_pt;
}

l_pt_t* create_l_pt(void){
  l_pt_t* l_pt = (l_pt_t*)malloc(sizeof(l_pt_t));
  if (l_pt == NULL) {
    perror("Failed to allocate memory for l_pt");
    return NULL;
  }

  for(int i = 0; i < NUM_LOWER_ENTRIES; i++){
    l_pt->entries[i].present = 0;
    l_pt->entries[i].read_write = 0;
    l_pt->entries[i].user_supervisor = 0;
    l_pt->entries[i].frame_number = 0;
    l_pt->entries[i].swapped = 0;
    l_pt->entries[i].swap_offset = 0;
  }
  return l_pt;
}

void free_page_table(u_pt_t* u_pt){
  if(u_pt == NULL) return;

  for(int i = 0; i < NUM_UPPER_ENTRIES; i++){
    u_pte_t* u_pte = &u_pt->entries[i];
    if(u_pte->present && u_pte->lower_table != NULL){
      free(u_pte->lower_table);
      u_pte->lower_table = NULL;
      u_pte->present = 0;
    }
  }
  free(u_pt);
}

int map_page(u_pt_t* u_pt, uint16_t vaddr, pid_t pid){
  uint8_t upper_index = vaddr >> 12;
  uint8_t lower_index = (vaddr >> 8) & 0xF;

  if(!u_pt->entries[upper_index].present){
    u_pt->entries[upper_index].lower_table = create_l_pt();
    u_pt->entries[upper_index].present = 1;
  }

  l_pt_t* l_pt = u_pt->entries[upper_index].lower_table;
  if(l_pt->entries[lower_index].present){
    return 0;
  }

  int frame_num = allocate_frame(pid, vaddr);
  if(frame_num == -1){
    // victim frame selection
    // LRU used
    int victim_frame = select_victim_frame();
    if(victim_frame == -1){
      perror("Failed to select victim frame");
      return -1;
    }

    pid_t victim_pid = get_pid_from_frame(victim_frame);
    u_pt_t* vic_u_pt = NULL;
    for (int i = 0; i < 10; i++) {
      if (victim_pid == global_pcb_ptrs[i]->pid) {
        vic_u_pt = global_pcb_ptrs[i]->page_table;
      }
    }

    // victim frame eviction
    printf("Victim frame: %d\n", victim_frame);
    if(swap_out_page(vic_u_pt, victim_frame, victim_pid, vaddr) != SWAP_SUCCESS){
      perror("Failed to swap out page");
      return -1;
    }

    frame_num = allocate_frame(pid, vaddr);
    if(frame_num == -1){ 
      perror("Failed to allocate frame after swapping out victim");
      return -1;
    }
  }

  l_pt->entries[lower_index].frame_number = frame_num;
  l_pt->entries[lower_index].present = 1;
  l_pt->entries[lower_index].read_write = 1;
  l_pt->entries[lower_index].user_supervisor = 1;

  return 0;
}

void unmap_page(u_pt_t* u_pt, tlb_t* tlb, uint16_t vaddr){
  uint8_t upper_index = vaddr >> 12;
  uint8_t lower_index = (vaddr >> 8) & 0xF;
  uint8_t vpn = (upper_index << 4) | lower_index;

  u_pte_t* u_pte = &u_pt->entries[upper_index];
  if(u_pte->present && u_pte->lower_table != NULL){
    l_pte_t* l_pte = &u_pte->lower_table->entries[lower_index];
    l_pte->present = 0;

    // TLB cache invalidation
    for(int i = 0; i < TLB_SIZE; i++){
      if(tlb->entries[i].valid && tlb->entries[i].vpn == vpn){
        tlb->entries[i].valid = 0;
        break;
      }
    }

  }
}

/*

  TLB HIT and Paging = 2
  TLB Miss and Paging = 1
  Page Fault = -1

  SWAP_IN_PAGE_FAULT = -2

 */

int translate_address(u_pt_t* u_pt, tlb_t* tlb, uint16_t vaddr, uint32_t* paddr, pid_t pid){
  // TLB lookup - Cache Hit
  if(tlb_lookup(tlb, vaddr, paddr, pid) == TLB_HIT) {
    int frame_number = *paddr >> 8;
    free_page_list[frame_number].last_used = get_currrent_time();
    return TLB_HIT_PAGING;
  }
  // Pagining Table lookup - Cache Miss
  uint8_t upper_index = (vaddr >> 12) & 0xF;
  uint8_t lower_index = (vaddr >> 8) & 0xF;
  uint8_t offset = vaddr & 0xFF;

  u_pte_t* u_pte = &u_pt->entries[upper_index];

  if(!u_pte->present){
    u_pte->lower_table = create_l_pt();
    if (u_pte->lower_table == NULL) {
      return -1;
    }
    u_pte->present = 1;
  }

  l_pt_t* l_pt = u_pte->lower_table;
  l_pte_t* l_pte = &l_pt->entries[lower_index];

  if (!l_pte->present) {
    if (l_pte->swapped) {
      // page swapp in
      if (swap_in_page(u_pt, vaddr, pid) != SWAP_SUCCESS) {
        return -1;
      }
    } else {
      if (map_page(u_pt, vaddr, pid) != 0) {
        return -1;
      }
    }
    l_pte = &l_pt->entries[lower_index];
  }

  *paddr = (l_pte->frame_number << 8) + offset;
  tlb_add_entry(tlb, vaddr, l_pte->frame_number, l_pte->read_write, l_pte->user_supervisor);

  free_page_list[l_pte->frame_number].last_used = get_currrent_time();

  return TLB_MISS_PAGING;
}

