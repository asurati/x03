// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_MUTEX_H
#define SYS_MUTEX_H

#include <sys/list.h>
#include <sys/thread.h>
#include <sys/spinlock.h>

// Define the structure here so that other structures can include
// struct spin_lock without making it a pointer.
struct mutex {
	int				lock;
	struct spin_lock		state_lock;
	struct list_head		wait_queue;
	struct thread			*next;
};

void	mutex_init(struct mutex *m);
void	mutex_lock(struct mutex *m);
void	mutex_unlock(struct mutex *m);
#endif
