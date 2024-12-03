#include "scheduling.h"
#include "queue.h"
#include "log.h"
#include "virtual_memory.h"

extern Queue *ready_queue;
extern Queue *wait_queue;

void initialize_scheduler(){
    ready_queue = createQueue();
    wait_queue = createQueue();
}

void FCFS_schedule(){
    if(!isEmpty(ready_queue)){
        Node *current_ready = ready_queue->head;
        if(current_ready != NULL){
            if(current_ready->pcb->flag == 0){
                cal_wait_time(current_ready);
            }
            if(current_ready->pcb->cpu_burst <= 0){
                if(current_ready->pcb->io_burst <= 0){
                    cal_turn_time(current_ready);
                    log_process_event(NULL, current_ready, "Re-enqueued to Ready queue");
                    
                    current_ready->pcb->flag = 0;
                    current_ready->pcb->arrival_time = time_count;
                    
                    enqueue(ready_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 
                    current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                }
                else{
                    log_process_event(NULL, current_ready, "Moved to WAIT queue for I/O");
                    enqueue(wait_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 
                    0, current_ready->pcb->io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                }
            }        
            else{
                log_process_event(NULL, current_ready, "Running");    
                current_ready->pcb->cpu_burst--; 
            }
        }        
    }

    if(!isEmpty(wait_queue)){
        Node *current_wait = wait_queue->head;
        if(current_wait != NULL){
            if(current_wait->pcb->io_burst <= 0){
                if(current_wait->pcb->cpu_burst <= 0){
                    cal_turn_time(current_wait);
                
                    log_process_event(NULL, current_wait, "Re-enqueued to Ready queue");
                    current_wait->pcb->flag = 0;
                    current_wait->pcb->arrival_time = time_count;

                    enqueue(ready_queue, current_wait->pcb->pid, current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, 
                    current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, current_wait->pcb->arrival_time, time_quantum, current_wait->pcb->page_directory);
                    dequeue(wait_queue);
                }
            }   
            else{
                log_process_event(NULL, current_wait, "Running");    
                current_wait->pcb->io_burst--;
            }
        }
    }
}

void RR_schedule(){
    if (!isEmpty(ready_queue)) {
        Node *current_ready = ready_queue->head;
        if (current_ready != NULL) {
            if(current_ready->pcb->flag == 0){
                cal_wait_time(current_ready);
            }        
            if (current_ready->pcb->remaining_time == 0) {
                if (current_ready->pcb->cpu_burst <= 0) {
                    if(current_ready->pcb->io_burst <= 0){ 
                        cal_turn_time(current_ready);
                        
                        current_ready->pcb->flag = 0;
                        current_ready->pcb->arrival_time = time_count;

                        log_process_event(NULL, current_ready, "Re-enqueued to Ready queue");
                        enqueue(ready_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                        dequeue(ready_queue);
                    } 
                    else{
                        log_process_event(NULL, current_ready, "Moved to WAIT queue for I/O");
                        enqueue(wait_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 0, current_ready->pcb->io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                        dequeue(ready_queue);
                    }
                }
                else{
                    current_ready->pcb->flag = 0;
                    current_ready->pcb->arrival_time = time_count;

                    log_process_event(NULL, current_ready, "Re-enqueued to Ready queue");
                    enqueue(ready_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->cpu_burst, current_ready->pcb->io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                }
            }
            else if(current_ready->pcb->cpu_burst <= 0){
                if(current_ready->pcb->io_burst <= 0){
                    cal_turn_time(current_ready);

                    current_ready->pcb->flag = 0;
                    current_ready->pcb->arrival_time = time_count;
                    
                    log_process_event(NULL, current_ready, "Re-enqueued to Ready queue");
                    enqueue(ready_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                } 
                else{
                    log_process_event(NULL, current_ready, "Re-enqueued to Wait queue");
                    enqueue(wait_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 0, current_ready->pcb->io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                }
            } 
            else{
                log_process_event(NULL, current_ready, "Running");
                current_ready->pcb->cpu_burst--;
                current_ready->pcb->remaining_time--;
            }
        }
    }

    if(!isEmpty(wait_queue)){
        Node *current_wait = wait_queue->head;
        if (current_wait != NULL) {
            if(current_wait->pcb->io_burst <= 0){
                if(current_wait->pcb->cpu_burst <= 0){
                    cal_turn_time(current_wait);
                    
                    current_wait->pcb->flag = 0;
                    current_wait->pcb->arrival_time = time_count;

                    log_process_event(NULL, current_wait, "Re-enqueued to Ready queue");
                    enqueue(ready_queue, current_wait->pcb->pid, current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, current_wait->pcb->arrival_time, time_quantum, current_wait->pcb->page_directory);
                    dequeue(wait_queue);
                }
                else{
                    current_wait->pcb->flag = 0;
                    current_wait->pcb->arrival_time = time_count;
                    log_process_event(NULL, current_wait, "Re-enqueued from Wait queue to Ready queue");
                    enqueue(ready_queue, current_wait->pcb->pid, current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, current_wait->pcb->cpu_burst, 0, current_wait->pcb->arrival_time, time_quantum, current_wait->pcb->page_directory);
                    dequeue(wait_queue);
                }
            }
            else{
                log_process_event(NULL, current_wait, "Running");
                current_wait->pcb->io_burst--;
                current_wait->pcb->remaining_time--;
            }
        }
    }
}

void SJF_schedule(){
    if(!isEmpty(ready_queue)) {
        Node *current_ready = ready_queue->head;
        if(current_ready != NULL){
            if(current_ready->pcb->flag == 0){
                cal_wait_time(current_ready);
            }
            if(current_ready->pcb->cpu_burst <= 0){
                if(current_ready->pcb->io_burst <= 0){
                    cal_turn_time(current_ready);

                    log_process_event(NULL, current_ready, "Re-enqueued to Ready queue");
                    enqueue(ready_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 
                    current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                    sort_queue(ready_queue);
                }
                else{
                    log_process_event(NULL, current_ready, "Moved to WAIT queue for I/O");
                    enqueue(wait_queue, current_ready->pcb->pid, current_ready->pcb->original_cpu_burst, current_ready->pcb->original_io_burst, 0, current_ready->pcb->io_burst, 
                    current_ready->pcb->arrival_time, time_quantum, current_ready->pcb->page_directory);
                    dequeue(ready_queue);
                }
            }
            else{
                log_process_event(NULL, current_ready, "Running");    
                current_ready->pcb->cpu_burst--; 
            }        
        }        
    }
    if(!isEmpty(wait_queue)){
        Node *current_wait = wait_queue->head;
        if(current_wait != NULL){
            if(current_wait->pcb->io_burst <= 0){
                if(current_wait->pcb->cpu_burst <= 0){
                    cal_turn_time(current_wait);
                    
                    log_process_event(NULL, current_wait, "Re-enqueued to Ready queue");

                    current_wait->pcb->flag = 0;
                    current_wait->pcb->arrival_time = time_count;

                    enqueue(ready_queue, current_wait->pcb->pid, current_wait->pcb->original_cpu_burst, current_wait->pcb->original_io_burst, current_wait->pcb->original_cpu_burst, 
                    current_wait->pcb->original_io_burst, current_wait->pcb->arrival_time, time_quantum, current_wait->pcb->page_directory);
                    dequeue(wait_queue);
                    sort_queue(ready_queue);
                }
            }   
            else{
                log_process_event(NULL, current_wait, "Running");    
                current_wait->pcb->io_burst--;
            }
        }   
    }
}

void terminate_scheduler(){
    removeQueue(ready_queue);
    removeQueue(wait_queue);
}