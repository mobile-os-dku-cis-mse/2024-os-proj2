//
// Created by hochacha on 24. 12. 2.
//

#ifndef PAGE_LIST_H
#define PAGE_LIST_H
#include <stdint.h>
#include <stdio.h>
typedef struct frame{
  uint32_t frame_number;
  struct frame* next;
} frame_t;

frame_t* free_frame_list = NULL;

#endif //PAGE_LIST_H
