#include <stdlib.h>
#include <sys/types.h>

#include "iarrq.h"

void iarrq_init(iarrq *q, int cap)
{
	q->mem = malloc(sizeof(unsigned int) * cap);
	q->cap = cap;
	q->sz = 0;
	q->front = 0;
	q->back = -1;
}

void iarrq_destroy(iarrq *q)
{
	free(q->mem);
}

int iarrq_empty(iarrq *q)
{
	return q->sz == 0;
}

int iarrq_full(iarrq *q)
{
	return q->sz == q->cap;
}

void iarrq_push(iarrq *q, unsigned int val)
{
	q->mem[q->back = (q->back+1) % q->cap] = val;
	q->sz++;
}

unsigned int iarrq_peek(iarrq *q)
{
	return q->mem[q->front];
}

unsigned int iarrq_pop(iarrq *q)
{
	unsigned int iarr = q->mem[q->front];
	q->front = (q->front+1) % q->cap;
	q->sz--;
	
	return iarr;
}
