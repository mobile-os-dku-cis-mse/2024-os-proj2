//
// Created by hochacha on 24. 11. 6.
//

#ifndef UTILS_H
#define UTILS_H

#include "../pid_queue/pid_queue.h"

void launch_scheduler_and_worker(pcb_queue* ready_queue, int n_process);

#endif //UTILS_H
