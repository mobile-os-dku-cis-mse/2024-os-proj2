#include "common.h"
#include "scheduling.h"
#include "timer.h"
#include "queue.h"
#include "process.h"
#include "log.h"
#include "virtual_memory.h"
#include "replacement_policy.h"

Queue *ready_queue;
Queue *wait_queue;

Process **parent;

FreeFrameList free_frame_list;

int counter;
int wait_proc_count;
int turn_proc_count;

int time_quantum;

int time_count;

double wait_time;
double turnaround_time;
double total_wait_time;

int schedule_policy;
int replacement_policy;
int disk_policy;

int *physical_memory;
int *virtual_memory;

int memory_page_num;
int memory_frame_num;

int timer;

int disk[DISK_SIZE];
int disk_idx;

int tb1_hit = 0;
int tb2_hit = 0;
int swap_count;
int cow_count;

struct timeval start, end;
long long disk_time_ns;

int program_count = 0;

void scheduler_handler(int signo){
    if(signo == SIGALRM){
        switch(schedule_policy){
            case 1:
                FCFS_schedule(); demand_paging(&free_frame_list, ready_queue); break;
            case 2:
                RR_schedule();   demand_paging(&free_frame_list, ready_queue); break;
            case 3:
                SJF_schedule();  demand_paging(&free_frame_list, ready_queue); break;
            default:
                perror("No Scheduling");
                exit(EXIT_FAILURE);
        }
        log_queue_state("READY Queue", ready_queue);
        log_queue_state("WAIT Queue", wait_queue);
        time_count++;
        program_count++;
    }
}

int main(int argc, char *argv[]){
    if(argc < 4){  
        fprintf(stderr, "Usage-1: %s <SCHEDULING POLICY> <REPLACEMENT POLICY> <DISK POLICY>.\n", argv[0]);
        fprintf(stderr, "Usage-2: <SCHEDULING POLICY> -> '1' is FCFS Scheduling | '2' is Round Robin Scheduling | '3' is SJF Scheduling\n");
        fprintf(stderr, "Usage-3: <REPLACEMENT POLICY> -> '1' is LRU Policy | '2' is FIFO Policy\n");
        fprintf(stderr, "Usage-4: <DISK POLICY> -> '1' is File Disk | '2' is Array Disk\n");
        exit(EXIT_FAILURE);
    }

    schedule_policy = atoi(argv[1]);
    replacement_policy = atoi(argv[2]);
    disk_policy = atoi(argv[3]);

    time_quantum = 0;
    
    if(schedule_policy == 2){    
        printf("========= Round Robin Scheduling =========\n");
        printf("Input Prgram TIME QUANTUM: ");
        scanf("%d", &time_quantum);
        printf("==========================================\n");
    }

    counter = 0;
    time_count = 0;
    wait_proc_count = 0;
    turn_proc_count = 0;

    wait_time = 0.0;
    turnaround_time = 0.0;
    total_wait_time = 0.0;
    
    timer = 0;
    
    memset(disk, 0, sizeof(disk)); 
    disk_idx = 0;   

    swap_count = 0;
    cow_count = 0;
    disk_time_ns = 0.0;

    switch(schedule_policy){
        case 1: initialize_scheduling_log("Dump/schedule_FCFS_dump.txt"); break;
        case 2: initialize_scheduling_log("Dump/schedule_RR_dump.txt"); break;
        case 3: initialize_scheduling_log("Dump/schedule_SJF_dump.txt"); break;
        default: perror("No Scheduler dump\n"); exit(EXIT_FAILURE);
    }

    if(schedule_policy == 1 && replacement_policy == 1 && disk_policy == 1) initialize_virtual_log("Dump/lru_FCFS_disk_file.txt");
    else if(schedule_policy == 1 && replacement_policy == 1 && disk_policy == 2) initialize_virtual_log("Dump/lru_FCFS_disk_array.txt");
    else if(schedule_policy == 1 && replacement_policy == 2 && disk_policy == 1) initialize_virtual_log("Dump/fifo_FCFS_disk_file.txt");
    else if(schedule_policy == 1 && replacement_policy == 2 && disk_policy == 2) initialize_virtual_log("Dump/fifo_FCFS_disk_array.txt");
    else if(schedule_policy == 2 && replacement_policy == 1 && disk_policy == 1) initialize_virtual_log("Dump/lru_RR_disk_file.txt");
    else if(schedule_policy == 2 && replacement_policy == 1 && disk_policy == 2) initialize_virtual_log("Dump/lru_RR_disk_array.txt");
    else if(schedule_policy == 2 && replacement_policy == 2 && disk_policy == 1) initialize_virtual_log("Dump/fifo_RR_disk_file.txt");
    else if(schedule_policy == 2 && replacement_policy == 2 && disk_policy == 2) initialize_virtual_log("Dump/fifo_RR_disk_array.txt");
    else if(schedule_policy == 3 && replacement_policy == 1 && disk_policy == 1) initialize_virtual_log("Dump/lru_SJF_disk_file.txt");
    else if(schedule_policy == 3 && replacement_policy == 1 && disk_policy == 2) initialize_virtual_log("Dump/lru_SJF_disk_array.txt");
    else if(schedule_policy == 3 && replacement_policy == 2 && disk_policy == 1) initialize_virtual_log("Dump/fifo_SJF_disk_file.txt");
    else if(schedule_policy == 3 && replacement_policy == 2 && disk_policy == 2) initialize_virtual_log("Dump/fifo_SJF_disk_array.txt");

    switch(disk_policy){
        case 1: initialize_disk_log(DISK_FILE); break;
        case 2: printf("Array Disk not create Log file.\n"); break;
        default: perror("No Disk"); exit(EXIT_FAILURE);
    }

    physical_memory = NULL;
    virtual_memory = NULL;

    initialize_timer(scheduler_handler);
    initialize_scheduler();
    initialize_memory(&physical_memory, &virtual_memory);


    ready_queue = createQueue();
    wait_queue = createQueue();
    
    srand(time(NULL));

    parent = (Process**)malloc(sizeof(Process**)*MAX_PROCESSES);
    PageDirectory page_directory = initialize_page_directory();;
    free_frame_list = initialize_free_frame_list();
    
    for(int i = 0; i < MAX_PROCESSES; i++){
        int cpu_burst = rand() % BURST_LIMIT + 1;
        int io_burst = rand() % BURST_LIMIT + 1;

        
        parent[i] = create_process(i, cpu_burst, io_burst, time_quantum, &page_directory);

        enqueue(ready_queue, parent[i]->pid, parent[i]->original_cpu_burst, parent[i]->original_io_burst,
                parent[i]->cpu_burst, parent[i]->io_burst, 0, time_quantum, &page_directory);

        log_process_event(parent[i], NULL, "Created");
        printf("Created Process: PID = %d, CPU Burst = %d, IO Burst = %d\n",
               parent[i]->pid, parent[i]->cpu_burst, parent[i]->io_burst);
    }
    printf("\n");

    if(schedule_policy == 3) sort_queue(ready_queue);

    printf("Virtual Memory Program Running ...\n");
    start_timer();
    while(program_count < 10000) pause();

    free(physical_memory);
    free(virtual_memory);
    close_scheduling_log();
    close_virtual_log();
    stop_timer();
    terminate_scheduler();

    printf("All processes completed. Exiting.\n");

    printf("=================== RESULT ===================\n");
    printf("[1] SCHEDULING STATISTICS\n");
    printf("1. AVG.WAIT TIME : %0.3f(s)\n", total_wait_time / wait_proc_count);
    printf("2. AVG.TURNAROUND TIME : %0.3f(s)\n\n", turnaround_time / turn_proc_count);

    printf("[2] VIRTUAL MEMORY STATISTICS\n");
    printf("1. PAGE TABLE LEVEL 1 HIT RATE: %.3f\n", (double)tb1_hit / 1000);
    printf("2. PAGE TABLE LEVEL 2 HIT RATE: %.3f\n", (double)tb2_hit / 1000);
    printf("3. SWAPPING COUNT : %d\n", swap_count);
    printf("4. COPY ON WRITE COUNT: %d\n", cow_count);
    printf("5. DISK ACCESS TIME: %lld(ns)\n", disk_time_ns);
    // printf("6. MEMORY ACCESS TIME: ")
    printf("==============================================\n");

    return 0;
}