#ifndef __PCB_H
#define __PCB_H

#include <sys/types.h>

struct pcb
{
	pid_t pid;
	int idx;
	unsigned int **page_tbl;
};

typedef struct pcb pcb;

void pcb_init(pcb*, pid_t, int);
void pcb_reset(pcb*);
void pcb_destroy(pcb*);

#endif
