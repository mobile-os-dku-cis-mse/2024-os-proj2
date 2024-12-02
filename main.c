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
#define PAGE_COUNT 4096
#define PAGE_SIZE 256
#define ENTRY_COUNT 64

extern int msqid;
extern void run();

int logfd;
pcbq rqueue;
pcb cur;
int ticks;

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

unsigned int mem_translate(unsigned int vaddr)
{
	if (!cur.page_tbl)
		cur.page_tbl = calloc(ENTRY_COUNT, sizeof(unsigned int*));

	const unsigned int MASK_1 = 0xFC000;
	const unsigned int MASK_2 = 0x03F00;
	const unsigned int MASK_OFF = 0x000FF;

	unsigned int off1 = (vaddr & MASK_1) >> 14;
	unsigned int off2 = (vaddr & MASK_2) >> 8;
	unsigned int off = (vaddr & MASK_OFF);

	if (!cur.page_tbl[off1])
	{
		cur.page_tbl[off1] = malloc(ENTRY_COUNT * sizeof(unsigned int));
		memset(cur.page_tbl[off1], 0xFF, ENTRY_COUNT * sizeof(unsigned int));
	}

	if (cur.page_tbl[off1][off2] == -1)
	{
		if (page_ptr == PAGE_SIZE)
			return -1;

		cur.page_tbl[off1][off2] = page_list[page_ptr++];
	}

	return cur.page_tbl[off1][off2] + off;
}

int mem_read(unsigned int vaddr)
{
	unsigned int target = mem_translate(vaddr);

	if (target == -1);
		//printf("error");
	else
		return mem[target];
}

void mem_write(unsigned int vaddr, unsigned int val)
{
	unsigned int target = mem_translate(vaddr);

	if (target == -1);
		//printf("error");
	else
		mem[target] = val;
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
	{
		cur = pcbq_pop(&rqueue);
		kill(cur.pid, SIGTERM);

		for (int i = 0; i < ENTRY_COUNT; i++)
			free(cur.page_tbl[i]);
		free(cur.page_tbl);
	}

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

	kill(cur.pid, SIGCONT);

	struct msgbuf buf;
	msgrcv(msqid, &buf, sizeof(buf) - sizeof(long), getpid(), 0);

	for (int i = 0; i < 10; i++)
	{
		// write flag.
		if (buf.rwflag & (1 << i))
			mem_write(buf.vaddr[i], buf.rwval[i]);

		// read flag.
		else
			buf.rwval[i] = mem_read(buf.vaddr[i]);
	}

	if (!--remaining)
	{
		pcbq_push(&rqueue, cur);
		cur = pcbq_pop(&rqueue);
		remaining = TIME_QUANTUM;
	}
}

void loop()
{
	// set-up the first running process.
	cur = pcbq_pop(&rqueue);

	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 100)
	{
		pause();
		schedule();
	}

	disable_ticks();

	// session has finished; currenly running process is forced back into the queue.
	pcbq_push(&rqueue, cur);
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
