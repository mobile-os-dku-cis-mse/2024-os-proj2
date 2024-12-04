#ifndef __ILINKQ_H
#define __ILINKQ_H

struct ilinkq_node
{
	unsigned int val;
	struct ilinkq_node *link;
};

typedef struct ilinkq_node ilinkq_node;

struct ilinkq
{
	ilinkq_node *front;
	ilinkq_node *back;
};

typedef struct ilinkq ilinkq;

void ilinkq_init(ilinkq*);
int ilinkq_empty(ilinkq*);
void ilinkq_push(ilinkq*, unsigned int);
unsigned int ilinkq_peek(ilinkq*);
unsigned int ilinkq_pop(ilinkq*);

#endif
