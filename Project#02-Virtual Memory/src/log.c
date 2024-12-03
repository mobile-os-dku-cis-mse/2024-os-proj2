#include "log.h"

static FILE *scheduling_log_file = NULL;
static FILE *virtual_log_file = NULL;
static FILE *disk_log_file = NULL;

void initialize_scheduling_log(const char *filename){
    scheduling_log_file = fopen(filename, "w");

    if (scheduling_log_file == NULL){
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    fprintf(scheduling_log_file, "----- Starting Scheduling Log ... -----\n");
    fprintf(scheduling_log_file, "Scheduling Log initialized at: %s\n\n", get_timestamp());
}

void initialize_virtual_log(const char *filename){
    virtual_log_file = fopen(filename, "w");

    if (virtual_log_file == NULL){
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    fprintf(virtual_log_file, "----- Starting Virtual Log ... -----\n");
    fprintf(virtual_log_file, "Scheduling Log initialized at: %s\n\n", get_timestamp());
}

void initialize_disk_log(const char *filename){
    disk_log_file = fopen(filename, "w");

    if (disk_log_file == NULL){
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
    fprintf(disk_log_file, "----- Starting Disk Log ... -----\n");
    fprintf(disk_log_file, "Scheduling Log initialized at: %s\n\n", get_timestamp());
}

void cal_wait_time(Node *current_node){
    wait_proc_count++;
    current_node->pcb->flag = 1;
    current_node->pcb->start_time = time_count;
    wait_time = current_node->pcb->start_time - current_node->pcb->arrival_time; 
    total_wait_time += wait_time;
}

void cal_turn_time(Node *current_node){
    turn_proc_count++; 
    turnaround_time += time_count - current_node->pcb->arrival_time;
}

const char* get_timestamp(){
    static char buffer[20];
    time_t now = time(NULL);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buffer;
}

void log_process_event(const Process *process, const Node * current, const char *event){
    if(scheduling_log_file){
        if(current != NULL){
            fprintf(scheduling_log_file, "[%s] Process %d - %s\n\n", get_timestamp(), current->pcb->pid, event);
        }
        else{
            fprintf(scheduling_log_file, "[%s] Process %d - %s\n\n", get_timestamp(), process->pid, event);
        }
        
    }
}

void log_memory_state(const int va, const int pa, Node *node, bool first_table_fault, bool second_table_fault, int data, int idx, char *request, bool isSwap, bool isCow){
    int dir_idx = (va >> 22) & 0x3ff;
    int table_idx = (va >> 12) & 0x3ff;
    fprintf(virtual_log_file, "===========================================\n");
    if(virtual_log_file){
        fprintf(virtual_log_file, "[TIME TICk: %d] | [Process %d]\n", (counter)/2, node->pcb->pid);
        fprintf(virtual_log_file, "[%d] Virtual Address: 0x%x\n", idx, va);
        fprintf(virtual_log_file, "[%d] Physical Address: 0x%x\n", idx, pa);
          
        if(first_table_fault == true && second_table_fault == true){
            fprintf(virtual_log_file, "1-Table Fault : %s\n2-Table Fault : %s\n", "O", "O");
        }
        else if(first_table_fault == false && second_table_fault == true){
            fprintf(virtual_log_file, "1-Table Fault : %s\n2-Table Fault : %s\n", "X", "O");
            tb1_hit += 1;
        }
        else if(first_table_fault == true && second_table_fault == false){
            fprintf(virtual_log_file, "1-Table Fault : %s\n2-Table Fault : %s\n", "O", "X");
            tb2_hit += 1;
        }
        else if(first_table_fault == false && second_table_fault == false){
            fprintf(virtual_log_file, "1-Table Fault : %s\n2-Table Fault : %s\n", "X", "X");
            tb1_hit += 1;
            tb2_hit += 1;
        }
        
        if(isSwap == true){
            fprintf(virtual_log_file, "Swapping : %s\n", "O");
            swap_count += 1;
        }
        else fprintf(virtual_log_file, "Swapping : %s\n", "X");
        
        if(isCow == true){
            fprintf(virtual_log_file, "Copy On Write : %s\n", "O");
            cow_count += 1;
        }
        else fprintf(virtual_log_file, "Copy On Write : %s\n", "X");
        fprintf(virtual_log_file, "Matching Info : [Page %d - Frame %d]\n", table_idx, (node->pcb->page_directory->page_table[dir_idx]->matching_frame[table_idx]));

        if(request == "R") fprintf(virtual_log_file, "Read Data from Memory: 0x%x\n", data);  
        else if(request == "W") fprintf(virtual_log_file, "Write Data to Memory: 0x%x\n", data);
        else fprintf(virtual_log_file, "No mapping, Only Swapping!\n");
    }
    fprintf(virtual_log_file, "===========================================\n\n");
}

void log_queue_state(const char *queue_name, const Queue *queue){
    if(counter % 2 == 0){    
        fprintf(scheduling_log_file, "/*==========================================================================*/\n");
    }

    if(scheduling_log_file){
        fprintf(scheduling_log_file, "\n[TIME TICK: %d] | [%s] Queue State - %s\n", counter++/2, get_timestamp(), queue_name);
        
        if(!isEmpty(queue)){
            Node *current = queue->head;
            while (current != NULL){
                if(schedule_policy == 2){    
                    fprintf(scheduling_log_file, "Process ID: %d, CPU Burst: %d, IO Burst: %d, Remaining: %d\n",
                            current->pcb->pid, current->pcb->cpu_burst, current->pcb->io_burst, current->pcb->remaining_time);
                }
                else{    
                    fprintf(scheduling_log_file, "Process ID: %d, CPU Burst: %d, IO Burst: %d\n",
                            current->pcb->pid, current->pcb->cpu_burst, current->pcb->io_burst);
                }
                current = current->next;
            }
        }
        fprintf(scheduling_log_file, "\n");
    }
}

void close_scheduling_log(){
    if(scheduling_log_file){
        fprintf(scheduling_log_file, "\n----- End of Scheduling Log -----\n");
        fclose(scheduling_log_file);
        scheduling_log_file = NULL;
    }
}

void close_virtual_log(){
    if(virtual_log_file){
        fprintf(virtual_log_file, "\n----- End of Virtual Log -----\n");
        fclose(virtual_log_file);
        virtual_log_file = NULL;
    }
}