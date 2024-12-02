#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "wrap/wrap.h"

extern int msqid;

void run()
{
	while (1)
	{
		my_msgrcv(msqid, NULL, 0);
		printf("process[%d] did something\n", getpid());
	}
}
