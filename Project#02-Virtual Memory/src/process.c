#include "process.h"

Process* create_process(int pid, int cpu_burst, int io_burst, int time_tick, PageDirectory *page_directory){
    Process *new_process = (Process*)malloc(sizeof(Process));
    if(new_process == NULL){
        perror("Failed to create process");
        exit(EXIT_FAILURE);
    }

    new_process->pid = pid;
    new_process->original_cpu_burst = cpu_burst;
    new_process->original_io_burst = io_burst;
    new_process->cpu_burst = cpu_burst;
    new_process->io_burst = io_burst;
    new_process->state = NEW;
    new_process->remaining_time = time_tick;
    new_process->arrival_time = 0.0;
    new_process->start_time = 0.0;
    new_process->flag = 0;
    new_process->page_directory = page_directory;
    
    return new_process;
}

void update_process_state(Process *process, ProcessState new_state){
    if(process->state != new_state) process->state = new_state;
}

void terminate_process(Process *process){
    if(process){
        printf("Terminating process %d\n", process->pid);
        free(process);
    }
}