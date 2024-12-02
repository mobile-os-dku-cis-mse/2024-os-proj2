//
// Created by hochacha on 24. 11. 30.
//

#include "tlb.h"

void initialize_tlb(tlb_t* tlb) {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb->entries[i].valid = 0;
    }
    tlb->next_replace_index = 0;
}




int tlb_lookup(tlb_t* tlb, uint16_t vaddr, uint32_t* paddr) {
    uint8_t upper_index = (vaddr >> 12) & 0xF;
    uint8_t lower_index = (vaddr >> 8) & 0xF;
    uint8_t offset = vaddr & 0xFF;
    uint8_t vpn = (upper_index << 4) | lower_index;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            *paddr = (tlb->entries[i].frame_number << 8) | offset;

            tlb->entries[i].timestamp = tlb->current_time++;
            tlb->entries[i].sca = 1;
            return TLB_HIT;
        }
    }
    return TLB_MISS;
}

void tlb_add_entry_fifo(tlb_t* tlb, uint16_t vaddr, uint32_t frame_number, int read_write, int user_supervisor) {
    uint8_t upper_index = (vaddr >> 12) & 0xF;
    uint8_t lower_index = (vaddr >> 8) & 0xF;
    uint8_t vpn = (upper_index << 4) | lower_index;

    int index = tlb->next_replace_index;

    tlb->entries[index].vpn = vpn;
    tlb->entries[index].frame_number = frame_number;
    tlb->entries[index].valid = 1;
    tlb->entries[index].read_write = read_write;
    tlb->entries[index].user_supervisor = user_supervisor;

    tlb->next_replace_index = (tlb->next_replace_index + 1) % TLB_SIZE;
}

void tlb_add_entry_lru(tlb_t* tlb, uint16_t vaddr, uint32_t frame_number, int read_write, int user_supervisor) {
    // vpn op
    uint8_t upper_index = (vaddr >> 12) & 0xF;
    uint8_t lower_index = (vaddr >> 8) & 0xF;
    uint8_t vpn = (upper_index << 4) | lower_index;

    // LRU 알고리즘을 위한 최소 timestamp를 가진 엔트리 찾기
    uint32_t min_timestamp = UINT32_MAX;
    int replace_index = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb->entries[i].valid) {
            replace_index = i;
            break;
        }
        if (tlb->entries[i].timestamp < min_timestamp) {
            min_timestamp = tlb->entries[i].timestamp;
            replace_index = i;
        }
    }

    // 엔트리 교체
    tlb->entries[replace_index].vpn = vpn;
    tlb->entries[replace_index].frame_number = frame_number;
    tlb->entries[replace_index].valid = 1;
    tlb->entries[replace_index].read_write = read_write;
    tlb->entries[replace_index].user_supervisor = user_supervisor;
    tlb->entries[replace_index].timestamp = tlb->current_time++;
    tlb->entries[replace_index].sca = 1;
}

void tlb_add_entry_sca(tlb_t* tlb, uint16_t vaddr, uint32_t frame_number, int read_write, int user_supervisor) {
    uint8_t upper_index = (vaddr >> 12) & 0xF;
    uint8_t lower_index = (vaddr >> 8) & 0xF;
    uint8_t vpn = (upper_index << 4) | lower_index;

    while (1) {
        int index = tlb->next_replace_index;

        if (!tlb->entries[index].valid) {
            // 빈 엔트리 발견
            tlb->entries[index].vpn = vpn;
            tlb->entries[index].frame_number = frame_number;
            tlb->entries[index].valid = 1;
            tlb->entries[index].read_write = read_write;
            tlb->entries[index].user_supervisor = user_supervisor;
            tlb->entries[index].sca = 0;

            tlb->next_replace_index = (index + 1) % TLB_SIZE;
            break;
        } else if (tlb->entries[index].sca == 0) {
            tlb->entries[index].vpn = vpn;
            tlb->entries[index].frame_number = frame_number;
            tlb->entries[index].valid = 1;
            tlb->entries[index].read_write = read_write;
            tlb->entries[index].user_supervisor = user_supervisor;
            tlb->entries[index].sca = 0;

            tlb->next_replace_index = (index + 1) % TLB_SIZE;
            break;
        } else {
            // 참조 비트가 1이면 0으로 리셋하고 다음 엔트리로 이동
            tlb->entries[index].sca = 0;
            tlb->next_replace_index = (index + 1) % TLB_SIZE;
        }
    }
}


void tlb_add_entry(tlb_t* tlb, uint16_t vaddr, uint32_t frame_number, int read_write, int user_supervisor) {
    switch (tlb->policy) {
    case FIFO:
        tlb_add_entry_fifo(tlb, vaddr, frame_number, read_write, user_supervisor);
        break;
    case LRU:
        tlb_add_entry_lru(tlb, vaddr, frame_number, read_write, user_supervisor);
        break;
    case SCA:
        tlb_add_entry_sca(tlb, vaddr, frame_number, read_write, user_supervisor);
        break;
    }
}