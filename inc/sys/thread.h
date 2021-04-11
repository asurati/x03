// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <sys/list.h>

struct thread;
typedef void (fn_thread)(void *p);
struct thread	*thread_get_current();
struct thread	*thread_create(fn_thread *fn, void *p);
struct thread	*thread_wakeup_one(struct list_head *wq);
void	thread_wakeup_all(struct list_head *wq);
void	thread_wait_and_switch(struct list_head *wq);
void	thread_idle();
int	thread_init();
#endif
