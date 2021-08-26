// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>

#include <dev/ioq.h>
#include <dev/con.h>

void ioq_init(struct ioq *ioq, fn_ioq_handler *req, fn_ioq_handler *res)
{
	list_init(&ioq->ior_head);
	spin_lock_init(&ioq->lock, IPL_SCHED);
	ioq->req = req;
	ioq->res = res;
}

void ior_init(struct ior *ior, struct ioq *ioq, int cmd, void *param,
	      pa_t param_pa)
{
	ior->cmd = cmd;
	ior->param = param;
	ior->ret = ERR_PENDING;
	ior->ioq = ioq;
	ior->param_pa = param_pa;
}

// IPL_THREAD
int ior_wait(struct ior *ior)
{
	struct ioq *ioq;

	ioq = ior->ioq;
	for (;;) {
		spin_lock(&ioq->lock);
		if (ior->ret != ERR_PENDING)
			break;
		spin_unlock(&ioq->lock);
	}
	spin_unlock(&ioq->lock);
	return ior->ret;
}

// IPL_THREAD
int ioq_queue_ior(struct ior *ior)
{
	int is_empty, err;
	struct list_head *head;
	struct ioq *ioq;

	ioq = ior->ioq;
	head = &ioq->ior_head;
	spin_lock(&ioq->lock);
	is_empty = list_is_empty(head);
	err = ERR_SUCCESS;
	if (is_empty)
		err = ioq->req(ior);
	if (!is_empty || !err)
		list_add_tail(head, &ior->entry);
	spin_unlock(&ioq->lock);
	return err;
}

// IPL_SOFT
int ioq_complete_ior(struct ioq *ioq)
{
	struct list_head *e, *head;
	struct ior *ior;
	int err;

	head = &ioq->ior_head;

	spin_lock(&ioq->lock);
	assert(!list_is_empty(head));

	e = list_del_head(head);
	ior = list_entry(e, struct ior, entry);
	ior->ret = ioq->res(ior);
	assert(ior->ret != ERR_PENDING);

	for (;;) {
		if (list_is_empty(head))
			break;
		e = list_peek_head(head);
		ior = list_entry(e, struct ior, entry);
		err = ioq->req(ior);
		if (!err)
			break;
		assert(err != ERR_PENDING);
		ior->ret = err;
		list_del_head(head);
	}
	spin_unlock(&ioq->lock);
	return ior->ret;
}
