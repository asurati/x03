// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_MUTEX_H
#define SYS_MUTEX_H

#include <sys/list.h>
#include <sys/thread.h>

struct mutex {
	struct thread			*next;
	struct list_head		wq;
	int				value;
};

void	mutex_init(struct mutex *m);
void	mutex_lock(struct mutex *m);
void	mutex_unlock(struct mutex *m);
void	mutex_lock_preempt_disabled(struct mutex *m);
void	mutex_unlock_preempt_disabled(struct mutex *m);
#endif
