// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <sys/cpu.h>
#include <sys/list.h>

enum thread_state {
	THREAD_STATE_RUNNING,
	THREAD_STATE_READY,
	THREAD_STATE_WAITING,
};

struct thread {
	// sp, lr, 4-8, 10, 11
	reg_t				regs[9];

	enum thread_state		state;
	struct list_head		wait_entry;
};

typedef int fn_thread(void *p);
int	thread_create(fn_thread *fn, void *p, struct thread **out);
void	thread_setup_wait(struct list_head *wq);
void	thread_wait();
void	thread_unwait(struct thread *t);
#endif
