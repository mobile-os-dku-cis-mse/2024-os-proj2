#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "shared/shared.h"

extern int msqid;

void null_handler(int) {}

void run()
{
	srand(getpid());
	set_sa_handler(SIGCONT, null_handler);

	while (1)
	{
		pause();
		
		struct msgbuf buf;
		memset(&buf, 0, sizeof(buf));
		buf.mtype = getppid();

		for (int i = 0; i < 10; i++)
		{
			buf.vaddr[i] = rand() % MEM_SIZE;

			if (rand() % 2)
			{
				// 0 means read, 1 means write.
				buf.rwflag |= 1 << i;
				buf.rwval[i] = rand();
			}
		}

		msgsnd(msqid, &buf, sizeof(buf) - sizeof(long), 0);
	}
}
