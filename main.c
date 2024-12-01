#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "ds/pidq.h"
#include "wrap/wrap.h"
#define TIME_QUANTUM 10

int logfd;
int msqid;
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

	while (wait(NULL) > 0);

	close(logfd);
	msgctl(msqid, IPC_RMID, NULL);
	pidq_destroy(&rqueue);
	exit(0);
}

void spawn()
{
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

void setup()
{
	logfd = creat("log.txt", 0666);
	msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
	pidq_init(&rqueue, 10);
}

int main()
{
	setup();
	spawn();
	loop();
	cleanup();

	return 0;
}
