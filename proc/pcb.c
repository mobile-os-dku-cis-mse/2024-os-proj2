#include <stdlib.h>
#include <sys/types.h>

#include "pcb.h"
#include "../ds/ilinkq.h"
#include "../ds/iarrq.h"
#include "../shared/shared.h"

void pcb_init(pcb *b, pid_t pid, int idx)
{
	b->pid = pid;
	b->idx = idx;
	b->page_tbl = calloc(ENTRY_COUNT, sizeof(unsigned int*));
	ilinkq_init(&b->p1entryq);
	ilinkq_init(&b->p2entryq);
}

void pcb_reset(pcb *b)
{
	while (!ilinkq_empty(&b->p2entryq))
	{
		unsigned int pentry = ilinkq_pop(&b->p2entryq);
		unsigned int off1 = pentry >> 6;
		unsigned int off2 = pentry & 0x3F;
		unsigned int pframe = b->page_tbl[off1][off2];

		if (pframe & 0x80000000)
			iarrq_push(&spageq, pframe & 0x7FFFFFFF);
		else
			iarrq_push(&mpageq, pframe);
	}

	while (!ilinkq_empty(&b->p1entryq))
	{
		unsigned int off1 = ilinkq_pop(&b->p1entryq);

		free(b->page_tbl[off1]);
		b->page_tbl[off1] = NULL;
	}
}
