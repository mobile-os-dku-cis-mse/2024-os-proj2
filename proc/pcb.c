#include <stdlib.h>
#include <sys/types.h>

#include "pcb.h"
#include "../ds/iarrq.h"
#include "../shared/shared.h"

void pcb_init(pcb *b, pid_t pid, int idx)
{
	b->pid = pid;
	b->idx = idx;
	b->page_tbl = calloc(ENTRY_COUNT, sizeof(unsigned int*));
}

void pcb_reset(pcb *b)
{
	for (int i = 0; i < ENTRY_COUNT; i++)
	{
		if (!b->page_tbl[i])
			continue;

		for (int j = 0; j < ENTRY_COUNT; j++)
		{
			unsigned int pframe = b->page_tbl[i][j];

			if (pframe == -1)
				continue;

			if (pframe & 0x80000000)
				iarrq_push(&spageq, pframe & 0x7FFFFFFF);
			else
				iarrq_push(&mpageq, pframe);
		}

		free(b->page_tbl[i]);
		b->page_tbl[i] = NULL;
	}
}

void pcb_destroy(pcb *b)
{
	pcb_reset(b);
	free(b->page_tbl);
}
