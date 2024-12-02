#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "ds/pcbq.h"
#include "shared/shared.h"
#define TIME_QUANTUM 10

extern int msqid;
extern void run();

int logfd;
pcbq rqueue;
int ticks;

#define MEM_SIZE 1048576
#define PAGE_SIZE 256
#define PAGE_COUNT 4096
unsigned int *mem;
unsigned int *page_list;
unsigned int page_ptr;

void mem_init()
{
	mem = calloc(MEM_SIZE, sizeof(unsigned int));
	page_list = calloc(PAGE_COUNT, sizeof(unsigned int));

	for (int i = 1; i < PAGE_COUNT; i++)
		page_list[i] = page_list[i-1] + PAGE_SIZE;

	page_ptr = 0;
}

int mem_read(unsigned int addr)
{
	return mem[addr];
}

void mem_write(unsigned int addr, unsigned int val)
{
	mem[addr] = val;
}

void alarm_handler(int)
{
	ticks++;
}

void enable_ticks()
{
	struct itimerval itimerval = {{0, 10000}, {0, 10000}};
	setitimer(ITIMER_REAL, &itimerval, NULL);
}

void disable_ticks()
{
	struct itimerval itimerval = {{0, 0}, {0, 0}};
	setitimer(ITIMER_REAL, &itimerval, NULL);
}

void cleanup()
{
	while (!pcbq_empty(&rqueue))
		kill(pcbq_pop(&rqueue).pid, SIGTERM);

	while (wait(NULL) > 0);

	free(page_list);
	free(mem);
	pcbq_destroy(&rqueue);
	close(logfd);
	msgctl(msqid, IPC_RMID, NULL);

	exit(0);
}

void spawn()
{
	while (!pcbq_full(&rqueue))
	{
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			cleanup();
		}
		
		if (pid)
			pcbq_push(&rqueue, (pcb) {pid, NULL});
		else
			run();
	}
}

void schedule()
{
	static int remaining = TIME_QUANTUM;

	kill(pcbq_peek(&rqueue).pid, SIGCONT);

	// my_msgrcv(pidq_pop(&rqueue), ...); we need to get the memory access request!

	if (!--remaining)
	{
		pcbq_push(&rqueue, pcbq_pop(&rqueue));
		remaining = TIME_QUANTUM;
	}
}

void loop()
{
	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 100)
	{
		pause();
		schedule();
	}

	disable_ticks();
}

void setup()
{
	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	logfd = creat("log.txt", 0666);
	pcbq_init(&rqueue, 10);
}

int main()
{
	setup();
	spawn();
	mem_init();
	loop();
	cleanup();

	return 0;
}
