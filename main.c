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
	page_list = malloc(PAGE_COUNT * sizeof(unsigned int));

	for (int i = 0; i < PAGE_COUNT; i++)
		page_list[i] = i;
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
		dprintf(logfd, "\t\t-> new page table allocated\n");
	}

	if (cur.page_tbl[off1][off2] == -1)
	{
		if (page_ptr == PAGE_COUNT)
		{
			dprintf(logfd, "\t\t-> cannot assign new page frame; not enough memory\n");
			return -1;
		}

		cur.page_tbl[off1][off2] = page_list[page_ptr++];
		dprintf(logfd, "\t\t-> new page frame assigned\n");
	}

	unsigned int addr = (cur.page_tbl[off1][off2] << 8) + off;
	dprintf(logfd, "\t\t-> translated to physical address 0x%05X\n", addr);
	return addr;
}

int mem_read(unsigned int addr)
{
	dprintf(logfd, "\t\t-> [0x%05X] contains 0x%08X\n", addr, mem[addr]);
	return mem[addr];
}

void mem_write(unsigned int addr, unsigned int val)
{
	mem[addr] = val;
	dprintf(logfd, "\t\t-> 0x%08X written to [0x%05X]\n", val, addr);
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

	dprintf(logfd, "log message at tick %d\n", ticks);
	dprintf(logfd, "process[%d] gets cpu time\n", cur.pid);
	kill(cur.pid, SIGCONT);

	struct msgbuf buf;
	msgrcv(msqid, &buf, sizeof(buf) - sizeof(long), getpid(), 0);

	for (int i = 0; i < 10; i++)
	{
		dprintf(logfd, "\tvirtual address 0x%05X\n", buf.vaddr[i]);
		unsigned int addr = mem_translate(buf.vaddr[i]);

		if (addr == -1)
			continue;

		// write flag.
		if (buf.rwflag & (1 << i))
			mem_write(addr, buf.rwval[i]);

		// read flag.
		else
			mem_read(addr);
	}

	if (!--remaining)
	{
		pcbq_push(&rqueue, cur);
		cur = pcbq_pop(&rqueue);
		remaining = TIME_QUANTUM;
		dprintf(logfd, "context switched\n");
	}

	dprintf(logfd, "\n\n");
}

void loop()
{
	// set-up the first running process.
	cur = pcbq_pop(&rqueue);

	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 1000)
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
