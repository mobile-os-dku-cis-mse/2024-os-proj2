#pragma once

#include "OS_memory.h"

#define MAX_TICKS 10000

int translate_address(int *free_frames, int *free_frame_count, PageTable *page_table, int virtual_address);
