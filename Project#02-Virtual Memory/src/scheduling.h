#pragma once

#include "process.h"
#include "queue.h"
#include "virtual_memory.h"

void initialize_scheduler(); 
void FCFS_schedule();
void RR_schedule();
void SJF_schedule();
void terminate_scheduler();