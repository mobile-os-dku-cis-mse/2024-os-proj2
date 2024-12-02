#ifndef __PCBQ_H
#define __PCBQ_H

#include <sys/types.h>

typedef struct pcb
{
	pid_t pid;
	unsigned int **page_tbl;
} pcb;

typedef struct pcbq
{
	pcb *mem;
	int cap, sz;
	int front, back;
} pcbq;

void pcbq_init(pcbq*, int);
void pcbq_destroy(pcbq*);
int pcbq_empty(pcbq*);
int pcbq_full(pcbq*);
void pcbq_push(pcbq*, pcb);
pcb pcbq_peek(pcbq*);
pcb pcbq_pop(pcbq*);
pcb pcbq_at(pcbq*, int);

#endif
