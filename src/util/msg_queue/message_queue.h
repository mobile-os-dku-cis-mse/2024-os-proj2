//
// Created by hochacha on 24. 11. 7.
//

#ifndef MESSGE_QUEUE_H
#define MESSGE_QUEUE_H
#include "../pid_queue/pid_queue.h"

#define MESSAGE_TYPE_IO_FIN 0
#define MESSAGE_TYPE_IO_REQ 1
#define MESSAGE_TYPE_SCHD_TIME 2
#define MESSAGE_TYPE_VADDR_ACCESS 3

extern int msg_queue_id_sched;
extern int msg_queue_id_page;

typedef struct{
    long msg_t;
    pid_t pid;
    unsigned int io_time;
    int new_io_burst;
    int new_cpu_burst;
    int is_finished;
}__attribute__((packed)) io_msg;

typedef struct {
    long mtype;
    int time_alloc;
}time_alloc_msg;

typedef struct{
    int msg_t;
    pid_t pid;
    uint16_t vaddr;
}vaddr_t;

typedef struct{ 
    int msg_t; 
    pid_t pid;
    uint8_t buf;
}access_res_t;

int init_msg_queue();
void close_msg_queue();
int get_msg_queue_id();
#endif //MESSGE_QUEUE_H
