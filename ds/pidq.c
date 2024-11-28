#include <stdlib.h>
#include <sys/types.h>

#include "pidq.h"

void pidq_init(pidq *q, int cap)
{
	q->mem = malloc(sizeof(pid_t) * cap);
	q->cap = cap;
	q->sz = 0;
	q->front = 0;
	q->back = -1;
}

void pidq_destroy(pidq *q)
{
	free(q->mem);
}

int pidq_empty(pidq *q)
{
	return q->sz == 0;
}

int pidq_full(pidq *q)
{
	return q->sz == q->cap;
}

void pidq_push(pidq *q, pid_t val)
{
	q->mem[q->back = (q->back+1) % q->cap] = val;
	q->sz++;
}

pid_t pidq_peek(pidq *q)
{
	return q->mem[q->front];
}

pid_t pidq_pop(pidq *q)
{
	pid_t pid = q->mem[q->front];
	q->front = (q->front+1) % q->cap;
	q->sz--;
	
	return pid;
}

pid_t pidq_at(pidq *q, int off)
{
	return q->mem[(q->front + off) % q->cap];
}
