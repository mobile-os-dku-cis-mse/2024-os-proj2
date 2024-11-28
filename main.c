#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include "ds/pidq.h"
#include "wrap/wrap.h"

pidq rqueue;
int ticks;

void run()
{
	while (1);
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
	while (!pidq_empty(&rqueue))
		kill(pidq_pop(&rqueue), SIGTERM);

	pidq_destroy(&rqueue);
	exit(0);
}

void spawn()
{
	pidq_init(&rqueue, 10);

	while (!pidq_full(&rqueue))
	{
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			cleanup();
		}
		
		if (pid)
			pidq_push(&rqueue, pid);
		else
			run();
	}
}

void schedule()
{
	printf("scheduler invoked: %d\n", ticks);
}

void loop()
{
	my_sigaction(SIGALRM, alarm_handler);
	enable_ticks();

	while (ticks < 100)
	{
		pause();
		schedule();
	}

	disable_ticks();
}

int main()
{
	spawn();
	loop();
	cleanup();

	return 0;
}
