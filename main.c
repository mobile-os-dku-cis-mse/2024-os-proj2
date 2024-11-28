#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "ds/pidq.h"

pidq rqueue;

void run()
{
	while (1);
}

void schedule()
{
	return;
}

void poweroff()
{
	while (!pidq_empty(&rqueue))
		kill(pidq_pop(&rqueue), SIGTERM);

	pidq_destroy(&rqueue);

	exit(0);
}

void boot()
{
	pidq_init(&rqueue, 10);
	while (!pidq_full(&rqueue))
	{
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			poweroff();
		}
		
		if (pid)
			pidq_push(&rqueue, pid);
		else
			run();
	}

	schedule();
}

int main()
{
	boot();
	poweroff();

	return 0;
}
