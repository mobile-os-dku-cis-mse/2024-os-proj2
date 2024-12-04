#include <stdlib.h>

#include "ilinkq.h"

static ilinkq_node *ilinkq_node_new(unsigned int val)
{
	ilinkq_node *node = malloc(sizeof(ilinkq_node));
	node->val = val;
	node->link = NULL;

	return node;
}

void ilinkq_init(ilinkq *q)
{
	q->front = NULL;
	q->back = NULL;
}

int ilinkq_empty(ilinkq *q)
{
	return q->front == NULL;
}

void ilinkq_push(ilinkq *q, unsigned int val)
{
	ilinkq_node *node = ilinkq_node_new(val);

	if (q->back)
		q->back->link = node;
	else
		q->front = node;

	q->back = node;
}

unsigned int ilinkq_peek(ilinkq *q)
{
	return q->front->val;
}

unsigned int ilinkq_pop(ilinkq *q)
{
	ilinkq_node *node = q->front;
	unsigned int val = node->val;

	q->front = q->front->link;
	if (!q->front)
		q->back = NULL;

	free(node);
	return val;
}
