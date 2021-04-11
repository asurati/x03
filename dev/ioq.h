// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef L_DEV_IOQ_H
#define L_DEV_IOQ_H

#include <sys/list.h>
#include <sys/event.h>

struct ior {
	struct list_head		entry;
	struct event			event;
	int				ret;
	int				cmd;
	void				*p;
};

typedef void (fn_ioq_handler)(struct ior *ior);
struct ioq {
	struct list_head		ior_head;
	struct ior			*curr_ior;
	fn_ioq_handler			*request;
	fn_ioq_handler			*response;
	int				num_iors;
};

void	ioq_init(struct ioq *ioq, fn_ioq_handler *req, fn_ioq_handler *res);
void	ioq_complete_curr_ior(struct ioq *ioq);
void	ioq_queue_ior(struct ioq *ioq, struct ior *ior);
void	ior_init(struct ior *ior, int cmd, void *p);
int	ior_wait(struct ior *ior);
void	ior_signal(struct ior *ior, int ret);
int	ior_cmd(const struct ior *ior);
void	*ior_param(const struct ior *ior);
#endif
