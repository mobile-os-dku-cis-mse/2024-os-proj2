#pragma once

#include "common.h"

typedef struct PT PageTable;
typedef struct PD PageDirectory;

typedef enum{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef struct Proc{
    int pid;
    int original_cpu_burst;
    int original_io_burst;
    int cpu_burst;
    int io_burst;
    int remaining_time;
    int flag;
    
    double arrival_time;
    double start_time;

    PageDirectory *page_directory;
    ProcessState state;
} Process;

Process* create_process(int pid, int cpu_burst, int io_burst, int time_tick, PageDirectory *page_directory);
void update_process_state(Process *process, ProcessState new_state);
void terminate_process(Process *process);
