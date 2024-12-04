#ifndef __IARRQ_H
#define __IARRQ_H

struct iarrq
{
	unsigned int *mem;
	int cap, sz;
	int front, back;
};

typedef struct iarrq iarrq;

void iarrq_init(iarrq*, int);
void iarrq_destroy(iarrq*);
int iarrq_empty(iarrq*);
int iarrq_full(iarrq*);
void iarrq_push(iarrq*, unsigned int);
unsigned int iarrq_peek(iarrq*);
unsigned int iarrq_pop(iarrq*);

#endif
