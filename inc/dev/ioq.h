// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_IOQ_H
#define DEV_IOQ_H

#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/spinlock.h>

struct ior;
typedef int fn_ioq_handler(struct ior *ior);
struct ioq {
	struct list_head		ior_head;
	struct spin_lock		lock;
	fn_ioq_handler			*req;
	fn_ioq_handler			*res;
};

struct ior {
	struct list_head		entry;
	struct ioq			*ioq;
	int				cmd;
	int				ret;
	void				*param;
	pa_t				param_pa;
};

static inline
int ior_cmd(const struct ior *ior)
{
	return ior->cmd;
}

static inline
int ior_ret(const struct ior *ior)
{
	return ior->ret;
}

static inline
pa_t ior_param_pa(const struct ior *ior)
{
	return ior->param_pa;
}

static inline
void *ior_param(const struct ior *ior)
{
	return (void *)ior->param;
}

void	ioq_init(struct ioq *ioq, fn_ioq_handler *req, fn_ioq_handler *res);
void	ior_init(struct ior *ior, struct ioq *ioq, int cmd, void *param,
		 pa_t pa);
int	ior_wait(struct ior *ior);
int	ioq_queue_ior(struct ior *ior);
int	ioq_complete_ior(struct ioq *ioq);
#endif
