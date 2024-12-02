#ifndef __SHARED_H
#define __SHARED_H

struct msgbuf
{
	long mtype;
	unsigned int rwflag;
	unsigned int vaddr[10];
};

int set_sa_handler(int, void (*)(int));

#endif
