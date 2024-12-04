#ifndef __PCB_H
#define __PCB_H

#include <sys/types.h>

#include "../ds/ilinkq.h"

struct pcb
{
	pid_t pid;
	unsigned int **page_tbl;
	ilinkq p1entryq;
	ilinkq p2entryq;
};

typedef struct pcb pcb;

void pcb_init(pcb*, pid_t);
void pcb_reset(pcb*);

#endif
