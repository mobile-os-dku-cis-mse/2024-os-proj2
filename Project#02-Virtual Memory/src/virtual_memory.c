#include "virtual_memory.h"

void initialize_memory(int **physical_memory, int **virtual_memory){
    uint32_t buf;
    uint32_t idx = 0;

    *physical_memory = (int*)malloc(PHYSICAL_SIZE);
    *virtual_memory = (int*)malloc(VIRTUAL_SIZE);

    if(*physical_memory == NULL){
        fprintf(stderr, "Memory allocation failed for physical_memory\n");
        exit(EXIT_FAILURE);
    }
    if(*virtual_memory == NULL){
        fprintf(stderr, "Memory allocation failed for virtual_memory\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen("input_file/input4.bin", "rb");
    if(fp == NULL){
        perror("Failed to open input4.bin!");
        exit(EXIT_FAILURE);
    }

    while(fread(&buf, 1, sizeof(int), fp) == 4){
        (*physical_memory)[idx / 4] = buf;
        idx += 4;
    }

    fclose(fp);
}

PageDirectory initialize_page_directory(){
    PageDirectory page_directory;
    page_directory.page_table = (PageTable**)malloc(PAGE_NUM * sizeof(PageTable *));

    if(page_directory.page_table == NULL){
        fprintf(stderr, "Memory Allocatiuon Failed for page directory\n");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < PAGE_NUM; i++) page_directory.page_table[i] = NULL;
    
    return page_directory;
}

FreeFrameList initialize_free_frame_list(void){
    FreeFrameList free_page;

    free_page.free_frame_list = (int*)malloc(FRAME_NUM * sizeof(int));

    if(free_page.free_frame_list == NULL){
        fprintf(stderr, "Memory allocation failed for free_frame_list\n");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < FRAME_NUM; i++) free_page.free_frame_list[i] = 1;
    
    return free_page;
}

int MMU(PageDirectory *page_directory, int virtual_address, bool *first_table_fault, bool *second_table_fault, bool *isRead, bool *isWrite){
    /* | FIRST TABLE IDX 10비트 | SECOND TABLE IDX 10비트 | OFFSET 12비트 */
    int dir_idx = (virtual_address >> 22) & 0x3ff; 
    int table_idx = (virtual_address >> 12) & 0x3ff; 
    int offset = virtual_address & 0xfff; 

    if(page_directory->page_table[dir_idx] == NULL){
        *first_table_fault = true;
        *second_table_fault = true;
        return dir_idx;
    }

    if (page_directory->page_table[dir_idx]->valid == NULL || page_directory->page_table[dir_idx]->valid[table_idx] == false) {
        *first_table_fault = false;
        *second_table_fault = true;
        return table_idx;
    }

    int mmu_frame_num = page_directory->page_table[dir_idx]->matching_frame[table_idx];
    page_directory->page_table[dir_idx]->valid[table_idx] = true;
    page_directory->page_table[dir_idx]->last_used[table_idx] = ++timer;
    *isRead = page_directory->page_table[dir_idx]->isRead;
    *isWrite = page_directory->page_table[dir_idx]->isWrite;
    return (mmu_frame_num * PAGE_SIZE + offset) / 4;
}

int handle_page_fault(PageDirectory *page_directory, FreeFrameList *free_list, int virtual_address, int pid){
    int fault_dir_idx = (virtual_address >> 22) & 0x3ff;   
    int fault_table_idx = (virtual_address >> 12) & 0x3ff; 

    if(page_directory->page_table[fault_dir_idx] == NULL){
        page_directory->page_table[fault_dir_idx] = (PageTable *)malloc(sizeof(PageTable));

        if(page_directory->page_table[fault_dir_idx] == NULL){
            fprintf(stderr, "Page table allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        page_directory->page_table[fault_dir_idx]->matching_frame = (int *)malloc(sizeof(int) * PAGE_NUM);
        page_directory->page_table[fault_dir_idx]->valid = (bool *)malloc(sizeof(bool) * PAGE_NUM);
        page_directory->page_table[fault_dir_idx]->last_used = (int *)malloc(sizeof(int) * PAGE_NUM);

        if(page_directory->page_table[fault_dir_idx]->matching_frame == NULL || page_directory->page_table[fault_dir_idx]->valid == NULL || page_directory->page_table[fault_dir_idx]->last_used == NULL){
            fprintf(stderr, "Fail to allocation Page Table Entry\n");
            exit(EXIT_FAILURE);
        }

        for(int i = 0 ; i < PAGE_NUM; i++) page_directory->page_table[fault_dir_idx]->matching_frame[i] = -1;
        for(int i = 0 ; i < PAGE_NUM; i++) page_directory->page_table[fault_dir_idx]->valid[i] = false;
        for(int i = 0 ; i < PAGE_NUM; i++) page_directory->page_table[fault_dir_idx]->last_used[i] = 0;

        page_directory->page_table[fault_dir_idx]->isRead = true;
        page_directory->page_table[fault_dir_idx]->isWrite = false;
    }

    if(page_directory->page_table[fault_dir_idx]->valid[fault_table_idx] == false){
        for(int i = 0; i < FRAME_NUM; i++){
            if(free_list->free_frame_list[i] == 1){
                page_directory->page_table[fault_dir_idx]->matching_frame[fault_table_idx] = i;
                page_directory->page_table[fault_dir_idx]->valid[fault_table_idx] = true;
                page_directory->page_table[fault_dir_idx]->last_used[fault_table_idx] = ++timer;

                free_list->free_frame_list[i] = 0;

                if(replacement_policy == 2) fifo_enqueue(fault_dir_idx, fault_table_idx);
               
                return FAULT_CODE_2;
            }
        }
    }
    
    gettimeofday(&start, NULL);
    int isFindDisk = find_from_disk(pid, fault_dir_idx, fault_table_idx);
    gettimeofday(&end, NULL);
    disk_time_ns += (end.tv_sec - start.tv_sec) * 1000000000LL + 
                    (end.tv_usec - start.tv_usec) * 1000;

    if(isFindDisk != -1) return FAULT_CODE_1;

    return FAULT_CODE_0;
}

int copy_page(PageDirectory *page_directory, int virtual_address, FreeFrameList *free_list, int pid){
    int fault_dir_idx = (virtual_address >> 22) & 0x3ff;    
    int fault_table_idx = (virtual_address >> 12) & 0x3ff;  

    if(page_directory->page_table[fault_dir_idx] == NULL || page_directory->page_table[fault_dir_idx]->valid[fault_table_idx] == false){
        fprintf(stderr, "Copy page called for invalid page!\n");
        exit(EXIT_FAILURE);
    }

    int original_frame = page_directory->page_table[fault_dir_idx]->matching_frame[fault_table_idx];
    page_directory->page_table[fault_dir_idx]->last_used[fault_table_idx] = ++timer;

    int new_frame = -1;
    
    for(int i = 0; i < FRAME_NUM; i++){
        if(free_list->free_frame_list[i] == 1){
            new_frame = i;
            free_list->free_frame_list[i] = 0;
            break;
        }
    }
    
    if(new_frame == -1) new_frame = swapping(page_directory, free_list, virtual_address, pid);

    page_directory->page_table[fault_dir_idx]->matching_frame[fault_table_idx] = new_frame;
    page_directory->page_table[fault_dir_idx]->valid[fault_table_idx] = true;
    page_directory->page_table[fault_dir_idx]->isRead = true;
    page_directory->page_table[fault_dir_idx]->isWrite = true;
    page_directory->page_table[fault_dir_idx]->last_used[fault_table_idx] = ++timer;

    return new_frame;
}

int swapping(PageDirectory *page_directory, FreeFrameList *free_list, int virtual_address, int pid){
    int replace_dir_idx = -1;
    int replace_table_idx = -1;
    int fault_dir_idx = (virtual_address >> 22) & 0x3ff;      
    int fault_table_idx = (virtual_address >> 12) & 0x3ff; 

    switch (replacement_policy){
        case 1: find_lru_page(page_directory, &replace_dir_idx, &replace_table_idx); break;
        case 2: find_fifo_page(&replace_dir_idx, &replace_table_idx); break;
        default: perror("No Replacement"); break;
    }

    if(replace_dir_idx == -1 || replace_table_idx == -1){
        fprintf(stderr, "No page available to swap out!\n");
        exit(EXIT_FAILURE);
    }

    int frame_to_replace = page_directory->page_table[replace_dir_idx]->matching_frame[replace_table_idx];
    int *frame_data = &physical_memory[frame_to_replace * PAGE_SIZE / 4];
    
    free_list->free_frame_list[frame_to_replace] = 1;
    
    page_directory->page_table[fault_dir_idx]->matching_frame[fault_table_idx] = frame_to_replace;
    page_directory->page_table[fault_dir_idx]->valid[fault_table_idx] = true;
    page_directory->page_table[fault_dir_idx]->last_used[fault_table_idx] = ++timer;
    page_directory->page_table[fault_dir_idx]->isRead = true;
    page_directory->page_table[fault_dir_idx]->isWrite = false;

    if(replacement_policy == 2) fifo_enqueue(fault_dir_idx, fault_table_idx);
    
    gettimeofday(&start, NULL);
    write_page_to_disk(page_directory, free_list, pid, replace_dir_idx, replace_table_idx, frame_data);
    gettimeofday(&end, NULL);
    disk_time_ns += (end.tv_sec - start.tv_sec) * 1000000000LL + 
                    (end.tv_usec - start.tv_usec) * 1000;

    return frame_to_replace;
}

void find_lru_page(PageDirectory *page_directory, int *replace_dir_idx, int *replace_table_idx){
    lru_replacement(page_directory, replace_dir_idx, replace_table_idx);
}

void find_fifo_page(int *replace_dir_idx, int *replace_table_idx){
    if(is_fifo_empty()){
        *replace_dir_idx = -1;
        *replace_table_idx = -1;
        return;
    }
    
    fifo_dequeue(replace_dir_idx, replace_table_idx);
}

void write_page_to_disk(PageDirectory *page_directory, FreeFrameList *free_list, int pid, int dir_idx, int table_idx, int *data){
    if(disk_policy == 1){
        FILE *disk_file = fopen(DISK_FILE, "a");
        if(!disk_file){
            fprintf(stderr, "Failed to open disk.txt for writing!\n");
            exit(EXIT_FAILURE);
        }
        fprintf(disk_file, "%d, %d, %d, 0x%x\n", pid, dir_idx, table_idx, *data);
        fclose(disk_file);
    }
    else if(disk_policy == 2){
        if(disk_idx >= DISK_SIZE){
            printf("DISK CAPACITY is OVER\n");
            exit(EXIT_FAILURE);
        }

        disk[disk_idx++] = pid;
        disk[disk_idx++] = dir_idx;
        disk[disk_idx++] = table_idx;
        disk[disk_idx++] = *data;
    }
    page_directory->page_table[dir_idx]->valid[table_idx] = false;
}

int find_from_disk(int pid, int dir_idx, int table_idx){
    int line_number = 0;
    int file_pid, file_dir_idx, file_table_idx;

    if(disk_policy == 1){
        FILE *disk_file = fopen(DISK_FILE, "r");
        if(!disk){
            fprintf(stderr, "Disk file not found!\n");
            exit(EXIT_FAILURE);
        }
        
        char line[256]; 
        while(fgets(line, sizeof(line), disk_file)){
            line_number++;
            char data[128];  
            data[0] = '\0'; 

            if(sscanf(line, "%d, %d, %d, %s", &file_pid, &file_dir_idx, &file_table_idx, data) == 4){
                if(file_pid == pid && file_dir_idx == dir_idx && file_table_idx == table_idx){
                    fclose(disk_file);  
                    return line_number;  
                }
            }
        }
        fclose(disk_file); 
    }
    else if(disk_policy == 2){
        for(int i = 0; i < DISK_SIZE; i += 4){
            line_number++;  
            file_pid = disk[i];
            file_dir_idx = disk[i+1];
            file_table_idx = disk[i+2];
            int data = disk[i+3]; 

            if(file_pid == pid && file_dir_idx == dir_idx && file_table_idx == table_idx){
                return line_number;
            }
        }
    }
    return -1; 
}

int read_page_from_disk(int line_num) {
    int data = 0;

    if(disk_policy == 1){
        FILE *disk_file = fopen(DISK_FILE, "r");
        FILE *temp = fopen(TEMP_FILE, "w");
        if (!disk_file || !temp) {
            fprintf(stderr, "Error opening file!\n");
            exit(EXIT_FAILURE);
        }

        char line[256];
        int current_line = 0;
        int data = 0;
        int found = 0;

        fseek(disk_file, line_num, SEEK_SET);
        if(fgets(line, sizeof(line), disk_file)){
            if(sscanf(line, "%*d, %*d, %*d, %x", &data) == 1) found = 1;
            fputs(line, temp);
        }

        fclose(disk_file);
        fclose(temp);

        if(found){
            remove(DISK_FILE);
            rename(TEMP_FILE, DISK_FILE);
            return data;
        } 
        else{
            remove(TEMP_FILE);
            return EOF;
        }
    }
    else if(disk_policy == 2){
        data = disk[line_num + 3];
        disk[line_num] = 0; disk[line_num+1] = 0; disk[line_num+2] = 0; disk[line_num+3] = 0;
        return data;
    }
}

void demand_paging(FreeFrameList *free_frame_list, Queue * queue){
    Node *current_process = queue->head;
    int pid = current_process->pcb->pid;

    for(int i = 0; i < MAX_PROCESSES; i++){
        int virtual_address = rand() % 0x800000;
        int memory_data = 0;
        int write_data = 0;
        int fault_num = -1;
        
        char * request = NULL;

        bool first_table_fault = false;
        bool second_table_fault = false;
        bool isRead = false;
        bool isWrite = false;
        bool isSwap = false;
        bool isCow = false;
        
        int dir_idx = (virtual_address >> 22) & 0x3ff;
        int table_idx = (virtual_address >> 12) & 0x3ff;
        int physical_address = MMU(current_process->pcb->page_directory, virtual_address, &first_table_fault, &second_table_fault, &isRead, &isWrite);

        if(first_table_fault == false && second_table_fault == false){
            memory_data = physical_memory[physical_address];
        }
        else{
            fault_num = handle_page_fault(current_process->pcb->page_directory, free_frame_list, virtual_address, pid);
            
            if(fault_num == FAULT_CODE_2){
                physical_address = MMU(current_process->pcb->page_directory, virtual_address, &first_table_fault, &second_table_fault, &isRead, &isWrite);
                memory_data = physical_memory[physical_address]; 
            }
            else if(fault_num == FAULT_CODE_1){
                gettimeofday(&start, NULL);
                int line_num = find_from_disk(pid, dir_idx, table_idx);
                int disk_data = read_page_from_disk(line_num);
                gettimeofday(&end, NULL);
                disk_time_ns += (end.tv_sec - start.tv_sec) * 1000000000LL + 
                                (end.tv_usec - start.tv_usec) * 1000;

                request = "W";
                swapping(current_process->pcb->page_directory, free_frame_list, virtual_address, pid);            
                isSwap = true;
                physical_address = MMU(current_process->pcb->page_directory, virtual_address, &first_table_fault, &second_table_fault, &isRead, &isWrite);
                physical_memory[physical_address] = disk_data; 
                log_memory_state(virtual_address, physical_address, current_process, first_table_fault, second_table_fault, write_data, i, request, isSwap, isCow);
                continue;
            }
            else if(fault_num == FAULT_CODE_0){
                swapping(current_process->pcb->page_directory, free_frame_list, virtual_address, pid);    
                isSwap = true;
            }
        }
        
        if(memory_data == 0){ 
            request = "W";
            write_data = memory_data + (i+1);
            
            if(isWrite == true){
                physical_memory[physical_address] = write_data;
            }
            else{ 
                int new_frame = copy_page(current_process->pcb->page_directory, virtual_address, free_frame_list, pid);
                physical_address = (new_frame * PAGE_SIZE + (virtual_address & 0xfff)) / 4;
                physical_memory[physical_address] = write_data; 
                isCow = true; 
            }
            log_memory_state(virtual_address, physical_address, current_process, first_table_fault, second_table_fault, write_data, i, request, isSwap, isCow);
        }   
        else{
            request = "R";         
            log_memory_state(virtual_address, physical_address, current_process, first_table_fault, second_table_fault, &physical_memory[physical_address], i, request, isSwap, isCow);
        }
    }
    timer++;
}