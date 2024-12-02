#ifndef __SHARED_H
#define __SHARED_H

#define MEM_SIZE 1048576

struct msgbuf
{
	long mtype;
	unsigned int rwflag;
	unsigned int rwval[10];
	unsigned int vaddr[10];
};

int set_sa_handler(int, void (*)(int));

#endif
