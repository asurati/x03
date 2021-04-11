// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/event.h>

#include "ioq.h"

void ioq_init(struct ioq *ioq, fn_ioq_handler *req, fn_ioq_handler *res)
{
	list_init(&ioq->ior_head);
	ioq->request = req;
	ioq->response = res;
	ioq->num_iors = 0;
}

void ior_init(struct ior *ior, int cmd, void *p)
{
	event_init(&ior->event);
	ior->ret = ERR_SUCCESS;
	ior->cmd = cmd;
	ior->p = p;
}

int ior_wait(struct ior *ior)
{
	event_wait(&ior->event);
	return ior->ret;
}

// Should be called at IPL_SOFT.
void ior_signal(struct ior *ior, int ret)
{
	ior->ret = ret;
	event_set_preempt_disabled(&ior->event);
}

int ior_cmd(const struct ior *ior)
{
	return ior->cmd;
}

void *ior_param(const struct ior *ior)
{
	return ior->p;
}

// Should be called at IPL_SOFT.
void ioq_complete_curr_ior(struct ioq *ioq)
{
	struct list_head *e;

	assert(ioq->curr_ior);

	ioq->response(ioq->curr_ior);

	ioq->curr_ior = NULL;
	--ioq->num_iors;
	if (ioq->num_iors == 0) return;

	assert(!list_is_empty(&ioq->ior_head));
	e = list_del_head(&ioq->ior_head);
	ioq->curr_ior = container_of(e, struct ior, entry);
	ioq->request(ioq->curr_ior);
}

// Should be called at IPL_THREAD.
void ioq_queue_ior(struct ioq *ioq, struct ior *ior)
{
	cpu_disable_preemption();
	++ioq->num_iors;
	if (ioq->num_iors == 1) {
		ioq->curr_ior = ior;
		ioq->request(ioq->curr_ior);
	} else {
		list_add_tail(&ioq->ior_head, &ior->entry);
	}
	cpu_enable_preemption();
}
