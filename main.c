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
#define PROCESS_COUNT 10
extern void run();

int logfd;
int ticks;
pcb proc_arr[PROCESS_COUNT];
int proc_ptr;

void mem_init()
{
	mem = calloc(MEM_SIZE, sizeof(unsigned int));
	swap = calloc(MEM_SIZE, sizeof(unsigned int));
	pfnmap = malloc(MEM_SIZE * sizeof(unsigned int));
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
	if (iarrq_empty(&spageq))
	{
		dprintf(logfd, "\t\t-> cannot assign new page frame; not enough swap memory\n");
		return -1;
	}

	// find available page frame to swap out.
	unsigned int pfn;
	for (pfn = 0; pfn < PAGE_COUNT; pfn++)
	{
		if ((pfnmap[pfn] & 0xF) != proc_ptr)
			break;
	}

	unsigned int off1 = (pfnmap[pfn] & 0xFC00) >> 10;
	unsigned int off2 = (pfnmap[pfn] & 0x3F0) >> 4;
	int proc_idx = pfnmap[pfn] & 0xF;

	unsigned int src = proc_arr[proc_idx].page_tbl[off1][off2];
	unsigned int dst = iarrq_pop(&spageq);
	memcpy(swap+(dst<<8), mem+(src<<8), PAGE_SIZE * sizeof(unsigned int));
	proc_arr[proc_idx].page_tbl[off1][off2] = dst | 0x80000000;

	return pfn;
}

unsigned int mem_translate(unsigned int vaddr)
{
	unsigned int off1 = (vaddr & 0xFC000) >> 14;
	unsigned int off2 = (vaddr & 0x3F00) >> 8;
	unsigned int off = (vaddr & 0xFF);
	int sflag = 0;

	pcb *cur = &proc_arr[proc_ptr];
	if (!cur->page_tbl[off1])
	{
		cur->page_tbl[off1] = malloc(ENTRY_COUNT * sizeof(unsigned int));
		memset(cur->page_tbl[off1], 0xFF, ENTRY_COUNT * sizeof(unsigned int));
		ilinkq_push(&cur->p1entryq, off1);

		dprintf(logfd, "\t\t-> new page table allocated\n");
	}

	if (cur->page_tbl[off1][off2] == -1)
	{
		// actual pfn, virtual pfn.
		unsigned int pfn;

		if (iarrq_empty(&mpageq))
		{
			dprintf(logfd, "\t\t-> no available page frames; resorting to swap memory\n");

			// set swap flag.
			sflag = 1;
			pfn = mem_swap();

			if (pfn == -1)
				return -1;
		}
		else
			pfn = iarrq_pop(&mpageq);

		unsigned int vpfn = (off1 << 6) | off2;

		cur->page_tbl[off1][off2] = pfn;
		pfnmap[pfn] = (vpfn << 4) | proc_ptr;
		ilinkq_push(&cur->p2entryq, vpfn);

		dprintf(logfd, "\t\t-> new page frame assigned\n");
	}

	unsigned int addr = (cur->page_tbl[off1][off2] << 8) + off;
	dprintf(logfd, "\t\t-> translated to physical address 0x%05X%s\n", addr, sflag ? "[swap]" : "");
	return addr | (sflag << 31);
}

unsigned int mem_read(unsigned int vaddr)
{
	unsigned int addr = mem_translate(vaddr);

	if (addr == -1)
		return -1;

	int sflag = addr >> 31;
	addr &= 0x7FFFFFFF;

	dprintf(logfd, "\t\t-> [0x%05X%s] contains 0x%08X\n", addr, sflag ? "[swap]" : "", sflag ? swap[addr] : mem[addr]);
	return sflag ? swap[addr] : mem[addr];
}

void mem_write(unsigned int vaddr, unsigned int val)
{
	unsigned int addr = mem_translate(vaddr);

	if (addr == -1)
		return;

	int sflag = addr >> 31;
	addr &= 0x7FFFFFFF;

	if (sflag)
		swap[addr] = val;
	else
		mem[addr] = val;

	dprintf(logfd, "\t\t-> 0x%08X written to [0x%05X%s]\n", val, addr, sflag ? "[swap]" : "");
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
	for (int i = 0; i < PROCESS_COUNT; i++)
	{
		kill(proc_arr[i].pid, SIGTERM);
		pcb_destroy(&proc_arr[i]);
	}

	while (wait(NULL) > 0);

	free(mem);
	free(swap);
	free(pfnmap);
	iarrq_destroy(&mpageq);
	iarrq_destroy(&spageq);
	close(logfd);
	msgctl(msqid, IPC_RMID, NULL);

	exit(0);
}

void spawn()
{
	for (int i = 0; i < PROCESS_COUNT; i++)
	{
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			cleanup();
		}
		
		if (pid)
			pcb_init(&proc_arr[i], pid, i);
		else
			run();
	}
}

void schedule()
{
	static int remaining = TIME_QUANTUM;
	static struct msgbuf buf;

	kill(proc_arr[proc_ptr].pid, SIGCONT);
	memset(&buf, 0, sizeof(buf));
	msgrcv(msqid, &buf, sizeof(buf) - sizeof(long), getpid(), 0);

	dprintf(logfd, "log message at tick %d\n", ticks);
	dprintf(logfd, "process[%d] gets cpu time, %d remaining\n", proc_arr[proc_ptr].pid, buf.burst);

	for (int i = 0; i < 10; i++)
	{
		dprintf(logfd, "\tvirtual address 0x%05X\n", buf.vaddr[i]);

		// write flag.
		if (buf.rwflag & (1 << i))
			mem_write(buf.vaddr[i], buf.rwval[i]);

		// read flag.
		else
			mem_read(buf.vaddr[i]);
	}

	if (!--buf.burst || !--remaining)
	{
		if (!buf.burst)
			pcb_reset(&proc_arr[proc_ptr]);

		proc_ptr = (proc_ptr + 1) % PROCESS_COUNT;
		remaining = TIME_QUANTUM;
		dprintf(logfd, "context switched\n");
	}

	dprintf(logfd, "\n\n");
}

void loop()
{
	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 1000)
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
