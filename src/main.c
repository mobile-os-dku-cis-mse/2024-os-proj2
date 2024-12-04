#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "entity/scheduler.h"
#include "util/process/process.h"
FILE* IO_result;
FILE* pcb_logs;
FILE* mem_access;

// shared memory p_mem declared

int main(int argc, char * arg[])
{
	int n_process = 10;
	pcb_queue* ready_queue_o = malloc(sizeof(pcb_queue) * n_process);
#ifdef PROJ1
	IO_result = freopen("scheduler_dump.txt", "w", stdout);
#elif PROJ2
	mem_access = fopen("memory_access_pattern.txt", "w");
#endif
	pcb_logs = fopen("Finished_PCB.txt", "w");
	launch_scheduler_and_worker(ready_queue_o, n_process);


	scheduler_run(ready_queue_o, 10);
	return 0;
}
