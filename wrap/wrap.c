#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "wrap.h"

int my_sigaction(int signum, void (*handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	return sigaction(signum, &sa, NULL);
}

int my_msgsnd(int msqid, pid_t target, int data)
{
	struct msgbuf msg;
	memset(&msg, 0, sizeof(msg));
	msg.mtype = target;
	msg.data = data;
	return msgsnd(msqid, &msg, sizeof(msg)-sizeof(long), 0);
}

ssize_t my_msgrcv(int msqid, int *data, int msgflg)
{
	struct msgbuf msg;
	memset(&msg, 0, sizeof(msg));
	ssize_t bytes = msgrcv(msqid, &msg, sizeof(msg)-sizeof(long), getpid(), msgflg);
	*data = msg.data;
	return bytes;
}
