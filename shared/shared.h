#ifndef __SHARED_H
#define __SHARED_H

#include "../ds/iarrq.h"

#define MEM_SIZE 1048576
#define PAGE_COUNT 4096
#define PAGE_SIZE 256
#define ENTRY_COUNT 64

struct msgbuf
{
	long mtype;
	unsigned int rwflag;
	unsigned int rwval[10];
	unsigned int vaddr[10];
	int burst;
};

extern int msqid;
extern unsigned int *mem;
extern unsigned int *swap;
extern unsigned int *pfnmap;
extern iarrq mpageq;
extern iarrq spageq;

int set_sa_handler(int, void (*)(int));

#endif
