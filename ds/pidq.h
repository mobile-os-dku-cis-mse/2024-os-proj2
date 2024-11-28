#ifndef __PIDQ_H
#define __PIDQ_H

#include <sys/types.h>

struct pidq
{
	pid_t *mem;
	int cap, sz;
	int front, back;
};

typedef struct pidq pidq;

void pidq_init(pidq*, int);
void pidq_destroy(pidq*);
int pidq_empty(pidq*);
int pidq_full(pidq*);
void pidq_push(pidq*, pid_t);
pid_t pidq_peek(pidq*);
pid_t pidq_pop(pidq*);
pid_t pidq_at(pidq*, int);

#endif
