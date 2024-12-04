#ifndef __PCBQ_H
#define __PCBQ_H

#include "../proc/pcb.h"

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

#endif
