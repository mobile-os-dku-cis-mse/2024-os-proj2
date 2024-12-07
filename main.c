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
#include "ds/iarrq.h"
#include "shared/shared.h"

#define TIME_QUANTUM 10
#define PROCESS_COUNT 10
extern void run();

int logfd;
int ticks;
pcb proc_arr[PROCESS_COUNT];
pcb *proc_ptr;
int proc_idx;

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

unsigned int mem_swap(unsigned int spfn)
{
	// find available page frame to swap out.
	unsigned int pfn;
	for (pfn = 0; pfn < PAGE_COUNT; pfn++)
	{
		if ((pfnmap[pfn] & 0xF) != proc_idx)
			break;
	}

	unsigned int svpfn = pfnmap[pfn] >> 4;
	unsigned int off1 = (pfnmap[pfn] & 0xFC00) >> 10;
	unsigned int off2 = (pfnmap[pfn] & 0x3F0) >> 4;
	pcb *sproc = proc_arr + (pfnmap[pfn] & 0xF);

	unsigned int src = sproc->page_tbl[off1][off2];
	memcpy(&swap[spfn << 8], &mem[pfn << 8], PAGE_SIZE * sizeof(unsigned int));
	sproc->page_tbl[off1][off2] = spfn | 0x80000000;

	dprintf(logfd, "\t\t-> swapped out process[%d]:0x%05X to 0x%05X[swap]\n", sproc->pid, svpfn << 8, spfn << 8);
	dprintf(logfd, "\t\t-> 0x%05X is now available\n", pfn << 8);
	return pfn;
}

unsigned int mem_translate(unsigned int vaddr)
{
	unsigned int vpfn = vaddr >> 8;
	unsigned int off1 = (vaddr & 0xFC000) >> 14;
	unsigned int off2 = (vaddr & 0x3F00) >> 8;
	unsigned int off = vaddr & 0xFF;

	// first level entry does not exist.
	if (!proc_ptr->page_tbl[off1])
	{
		proc_ptr->page_tbl[off1] = malloc(ENTRY_COUNT * sizeof(unsigned int));
		memset(proc_ptr->page_tbl[off1], 0xFF, ENTRY_COUNT * sizeof(unsigned int));
		dprintf(logfd, "\t\t-> new page table allocated\n");
	}

	unsigned int *pfn_ptr = proc_ptr->page_tbl[off1] + off2;
	unsigned int pfn = *pfn_ptr;
	
	// page frame number not assigned yet.
	if (pfn == -1)
	{
		if (iarrq_empty(&mpageq))
		{
			dprintf(logfd, "\t\t-> no available page frames; resorting to swap memory\n");

			if (iarrq_empty(&spageq))
			{
				dprintf(logfd, "\t\t-> cannot assign new page frame; not enough swap memory\n");
				return -1;
			}

			pfn = mem_swap(iarrq_pop(&spageq));
		}
		else
			pfn = iarrq_pop(&mpageq);

		dprintf(logfd, "\t\t-> new page frame assigned\n");
	}

	// page exists in swap memory.
	if (pfn & 0x80000000)
	{
		static unsigned int __spage[PAGE_SIZE];
		unsigned int spfn = pfn & 0x7FFFFFFF;

		dprintf(logfd, "\t\t-> page exists in swap section\n");

		if (iarrq_empty(&mpageq))
		{
			dprintf(logfd, "\t\t-> no page available in memory\n");
			memcpy(__spage, &swap[spfn << 8], PAGE_SIZE * sizeof(unsigned int));
			pfn = mem_swap(spfn);
			memcpy(&mem[pfn << 8], __spage, PAGE_SIZE * sizeof(unsigned int));
		}
		else
		{
			dprintf(logfd, "\t\t-> new page available in memory\n");
			pfn = iarrq_pop(&mpageq);
			memcpy(&mem[pfn << 8], &swap[spfn << 8], PAGE_SIZE * sizeof(unsigned int));
			iarrq_push(&spageq, spfn);
		}
		
		dprintf(logfd, "\t\t-> page frame reassigned\n");
	}

	*pfn_ptr = pfn;
	pfnmap[pfn] = (vpfn << 4) | proc_idx;

	unsigned int addr = (pfn << 8) | off;
	dprintf(logfd, "\t\t-> translated to physical address 0x%05X\n", addr);
	return addr;
}

unsigned int mem_read(unsigned int vaddr)
{
	unsigned int addr = mem_translate(vaddr);

	if (addr == -1)
		return -1;

	dprintf(logfd, "\t\t-> [0x%05X] contains 0x%08X\n", addr, mem[addr]);
	return mem[addr];
}

void mem_write(unsigned int vaddr, unsigned int val)
{
	unsigned int addr = mem_translate(vaddr);

	if (addr == -1)
		return;
	
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

	proc_ptr = proc_arr;
}

void schedule()
{
	printf("scheduler invoked, tick %d\n", ticks);

	static int remaining = TIME_QUANTUM;
	static struct msgbuf buf;

	kill(proc_ptr->pid, SIGCONT);
	memset(&buf, 0, sizeof(buf));
	msgrcv(msqid, &buf, sizeof(buf) - sizeof(long), getpid(), 0);

	dprintf(logfd, "log message at tick %d\n", ticks);
	dprintf(logfd, "process[%d] gets cpu time, %d remaining\n", proc_ptr->pid, buf.burst);

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
			pcb_reset(proc_ptr);

		proc_idx = (proc_idx + 1) % PROCESS_COUNT;
		proc_ptr = proc_arr + proc_idx;
		remaining = TIME_QUANTUM;
		dprintf(logfd, "context switched\n");
	}

	dprintf(logfd, "\n\n");
}

void loop()
{
	set_sa_handler(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 5000)
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
