#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAX_PROCESSES 10
#define BURST_LIMIT 10
#define TIME_TICK 2000 
#define DISK_SIZE 0x400000 // 4MB

#define DISK_FILE "Disk/disk.txt"
#define TEMP_FILE "Disk/temp.txt"

extern int counter;
extern int burst_limit;
extern int wait_proc_count;
extern int turn_proc_count;

extern int time_count;

extern int time_quantum;

extern double wait_time;
extern double turnaround_time;
extern double total_wait_time;

extern int schedule_policy;
extern int replacement_policy;
extern int disk_policy;

extern int * physical_memory;
extern int * virtual_memory;

extern int timer;

extern int disk[DISK_SIZE];
extern int disk_idx;

extern int tb1_hit;
extern int tb2_hit;
extern int swap_count;
extern int cow_count;
extern struct timeval start, end;
extern long long disk_time_ns;