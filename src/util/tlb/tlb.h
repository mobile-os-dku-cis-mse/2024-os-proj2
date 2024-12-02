//
// Created by hochacha on 24. 11. 30.
//

#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#define TLB_SIZE 16

typedef struct {
    uint8_t vpn;
    uint32_t frame_number;
    int valid;
    int read_write;
    int user_supervisor;

    uint32_t timestamp;
    int sca;
} tlb_entry_t;

typedef enum {
    FIFO,
    LRU,
    SCA
} tlb_replacement_policy_t;

typedef struct {
    tlb_entry_t entries[TLB_SIZE];
    int next_replace_index;
    tlb_replacement_policy_t policy;
    uint32_t current_time;
} tlb_t;

#define TLB_HIT 0
#define TLB_MISS -1

void initialize_tlb(tlb_t* tlb);
int tlb_lookup(tlb_t* tlb, uint16_t vaddr, uint32_t* paddr);
void tlb_add_entry(tlb_t* tlb, uint16_t vaddr, uint32_t frame_number, int read_write, int user_supervisor);

#endif //TLB_H
