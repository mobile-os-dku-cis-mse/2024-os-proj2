#ifndef __WRAP_H
#define __WRAP_H

#include <sys/types.h>

struct msgbuf
{
	long mtype;
	int data;
};

int my_sigaction(int, void(*)(int));
int my_msgsnd(int, pid_t, int);
ssize_t my_msgrcv(int, int*, int);

#endif
