//
// Created by hochacha on 24. 11. 30.
//

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "../tlb/tlb.h"
#include <sys/types.h>
#define PAGE_SIZE 256
#define PAGE_OFFSET_BITS 8
#define LOWER_INDEX_BITS 4
#define UPPER_INDEX_BITS 4

#define NUM_LOWER_ENTRIES (1 << LOWER_INDEX_BITS)
#define NUM_UPPER_ENTRIES (1 << UPPER_INDEX_BITS)

#define TLB_HIT_PAGING 2
#define TLB_MISS_PAGING 1
#define PAGE_FAULT (-1)
#define SWAP_IN_PAGE_FAULT (-2)

typedef struct{
  uint32_t frame_number;

  int present;
  int read_write;
  int user_supervisor;
  int dirty;
  int swapped; // 1: swapped, 0: not swapped
  uint32_t swap_offset;
} l_pte_t;

typedef struct{
  l_pte_t entries[NUM_LOWER_ENTRIES];
} l_pt_t;

typedef struct{
  l_pt_t* lower_table;
  int present;
} u_pte_t;

typedef struct{
  u_pte_t entries[NUM_UPPER_ENTRIES];
} u_pt_t;

u_pt_t* create_u_pt(void);
l_pt_t* create_l_pt(void);
int map_page(u_pt_t* u_pt, uint16_t vaddr, pid_t pid);
void unmap_page(u_pt_t* u_pt, tlb_t* tlb, uint16_t vaddr);
int translate_address(u_pt_t* u_pt, tlb_t* tlb, uint16_t vaddr, uint32_t* paddr, pid_t pid);
void free_page_table(u_pt_t* u_pt);

#endif //PAGING_H
