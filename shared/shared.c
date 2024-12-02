#include <string.h>
#include <signal.h>

#include "shared.h"

int msqid;

int set_sa_handler(int signum, void (*handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	return sigaction(signum, &sa, NULL);
}
