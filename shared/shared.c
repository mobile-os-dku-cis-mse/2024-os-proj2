#include <string.h>
#include <signal.h>

#include "shared.h"
#include "../ds/iarrq.h"

int msqid;
unsigned int *mem;
unsigned int *swap;
unsigned int *pfnmap;
iarrq mpageq;
iarrq spageq;

int set_sa_handler(int signum, void (*handler)(int))
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	return sigaction(signum, &sa, NULL);
}
