#include <stdlib.h>
#include <sys/types.h>

#include "../proc/pcb.h"
#include "pcbq.h"

void pcbq_init(pcbq *q, int cap)
{
	q->mem = malloc(sizeof(pcb) * cap);
	q->cap = cap;
	q->sz = 0;
	q->front = 0;
	q->back = -1;
}

void pcbq_destroy(pcbq *q)
{
	free(q->mem);
}

int pcbq_empty(pcbq *q)
{
	return q->sz == 0;
}

int pcbq_full(pcbq *q)
{
	return q->sz == q->cap;
}

void pcbq_push(pcbq *q, pcb b)
{
	q->mem[q->back = (q->back+1) % q->cap] = b;
	q->sz++;
}

pcb pcbq_peek(pcbq *q)
{
	return q->mem[q->front];
}

pcb pcbq_pop(pcbq *q)
{
	pcb b = q->mem[q->front];
	q->front = (q->front+1) % q->cap;
	q->sz--;
	
	return b;
}
