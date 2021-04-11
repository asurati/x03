// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_EVENT_H
#define SYS_EVENT_H

#include <sys/list.h>
#include <sys/thread.h>

struct event {
	struct list_head		wq;
	int				value;
};

void	event_init(struct event *e);
void	event_wait(struct event *e);
void	event_reset_preempt_disabled(struct event *e);
void	event_reset(struct event *e);
void	event_set_preempt_disabled(struct event *e);
void	event_set(struct event *e);
#endif
