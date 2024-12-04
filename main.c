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

#include "proc/pcb.h"
#include "ds/pcbq.h"
#include "ds/iarrq.h"
#include "ds/ilinkq.h"
#include "shared/shared.h"

#define TIME_QUANTUM 10
extern void run();

int logfd;
int ticks;
pcbq schedq;
pcb cur;

void mem_init()
{
	mem = calloc(MEM_SIZE, sizeof(unsigned int));
	swap = calloc(MEM_SIZE, sizeof(unsigned int));
	iarrq_init(&mpageq, PAGE_COUNT);
	iarrq_init(&spageq, PAGE_COUNT);

	for (int i = 0; i < PAGE_COUNT; i++)
	{
		iarrq_push(&mpageq, i);
		iarrq_push(&spageq, i);
	}
}

unsigned int mem_swap()
{

}

unsigned int mem_translate(unsigned int vaddr)
{
	unsigned int off1 = (vaddr & 0xFC000) >> 14;
	unsigned int off2 = (vaddr & 0x3F00) >> 8;
	unsigned int off = (vaddr & 0xFF);

	if (!cur.page_tbl[off1])
	{
		cur.page_tbl[off1] = malloc(ENTRY_COUNT * sizeof(unsigned int));
		memset(cur.page_tbl[off1], 0xFF, ENTRY_COUNT * sizeof(unsigned int));
		ilinkq_push(&cur.p1entryq, off1);

		dprintf(logfd, "\t\t-> new page table allocated\n");
	}

	if (cur.page_tbl[off1][off2] == -1)
	{
		if (iarrq_empty(&mpageq))
		{
			dprintf(logfd, "\t\t-> cannot assign new page frame; not enough memory\n");
			return -1;
		}

		cur.page_tbl[off1][off2] = iarrq_pop(&mpageq);
		ilinkq_push(&cur.p2entryq, (off1 << 6) | off2);

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
	while (!pcbq_empty(&schedq))
	{
		cur = pcbq_pop(&schedq);
		kill(cur.pid, SIGTERM);

		for (int i = 0; i < ENTRY_COUNT; i++)
			free(cur.page_tbl[i]);
		free(cur.page_tbl);
	}

	while (wait(NULL) > 0);

	free(mem);
	free(swap);
	iarrq_destroy(&mpageq);
	iarrq_destroy(&spageq);
	pcbq_destroy(&schedq);
	close(logfd);
	msgctl(msqid, IPC_RMID, NULL);

	exit(0);
}

void spawn()
{
	while (!pcbq_full(&schedq))
	{
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			cleanup();
		}
		
		if (pid)
		{
			pcb b;
			pcb_init(&b, pid);
			pcbq_push(&schedq, b);
		}
		else
			run();
	}
}

void schedule()
{
	static int remaining = TIME_QUANTUM;
	static struct msgbuf buf;

	kill(cur.pid, SIGCONT);
	memset(&buf, 0, sizeof(buf));
	msgrcv(msqid, &buf, sizeof(buf) - sizeof(long), getpid(), 0);

	dprintf(logfd, "log message at tick %d\n", ticks);
	dprintf(logfd, "process[%d] gets cpu time, %d remaining\n", cur.pid, buf.burst);

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

	if (!--buf.burst || !--remaining)
	{
		if (!buf.burst)
			pcb_reset(&cur);

		pcbq_push(&schedq, cur);
		cur = pcbq_pop(&schedq);
		remaining = TIME_QUANTUM;
		dprintf(logfd, "context switched\n");
	}

	dprintf(logfd, "\n\n");
}

void loop()
{
	// set-up the first running process.
	cur = pcbq_pop(&schedq);

	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 1000)
	{
		pause();
		schedule();
	}

	disable_ticks();

	// session has finished; currenly running process is forced back into the queue.
	pcbq_push(&schedq, cur);
}

void setup()
{
	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	logfd = creat("log.txt", 0666);
	pcbq_init(&schedq, 10);
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
