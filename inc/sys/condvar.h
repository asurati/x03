// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_CONDVAR_H
#define SYS_CONDVAR_H

#include <sys/list.h>
#include <sys/mutex.h>
#include <sys/spinlock.h>

struct cond_var {
	struct spin_lock		state_lock;
	struct list_head		wait_queue;
	int				signalled;
	int				num_waiters;
};

void	cond_var_init(struct cond_var *v);
void	cond_var_wait(struct cond_var *v, struct mutex *lock);
void	cond_var_signal(struct cond_var *v);
#endif
