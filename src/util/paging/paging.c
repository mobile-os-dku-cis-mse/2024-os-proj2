//
// Created by hochacha on 24. 11. 30.
//

#include "paging.h"
#include "../tlb/tlb.h"
#include <stdlib.h>
#include <stdio.h>
#include "../frame_list/frame_list.h"

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
  }
  return l_pt;
}

int map_page(u_pt_t* u_pt, uint16_t vaddr){
  uint8_t upper_index = vaddr >> 12;
  uint8_t lower_index = (vaddr >> 8) & 0xF;

  if(!u_pt->entries[upper_index].present){
    u_pt->entries[upper_index].lower_table = create_l_pt();
    u_pt->entries[upper_index].present = 1;
  }

  l_pt_t* l_pt = u_pt->entries[upper_index].lower_table;

  int frame_num = allocate_page();
  if(frame_num == -1){
    return -1;
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

 */

int translate_address(u_pt_t* u_pt, tlb_t* tlb, uint16_t vaddr, uint32_t* paddr){
  // TLB lookup - Cache Hit
  if(tlb_lookup(tlb, vaddr, paddr) == TLB_HIT) return TLB_HIT_PAGING;

  // Pagining Table lookup - Cache Miss
  uint8_t upper_index = (vaddr >> 12) & 0xF;
  uint8_t lower_index = (vaddr >> 8) & 0xF;
  uint8_t offset = vaddr & 0xFF;

  u_pte_t* u_pte = &u_pt->entries[upper_index];

  if(!u_pte->present){
    return PAGE_FAULT;
  }

  l_pt_t* l_pt = u_pte->lower_table;
  l_pte_t* l_pte = &l_pt->entries[lower_index];

  if(!l_pte->present){
    return PAGE_FAULT;
  }

  *paddr = (l_pte->frame_number << 8) + offset;
  tlb_add_entry(tlb, vaddr, l_pte->frame_number, l_pte->read_write, l_pte->user_supervisor);
  return TLB_MISS_PAGING;
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