#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "shared/shared.h"

extern int msqid;

void null_handler(int) {}

void run()
{
	set_sa_handler(SIGCONT, null_handler);

	while (1)
	{
		pause();
		printf("process[%d] did something\n", getpid());
	}
}
