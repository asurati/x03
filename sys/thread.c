// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/thread.h>

int thread_idle_thread()
{
	int err;
	struct thread *t;
	int kmain();

	t = cpu_get_curr_thread();
	t->state = THREAD_STATE_RUNNING;

	err = kmain();
	if (err)
		return err;

	for (;;)
		cpu_yield();
}

// Called at IPL_SCHED
void thread_unwait(struct thread *t)
{
	struct list_head *rq;
	assert(t->state == THREAD_STATE_WAITING);

	rq = cpu_get_ready_queue();
	t->state = THREAD_STATE_READY;
	list_add_tail(rq, &t->wait_entry);
}

// Called at IPL_SCHED
void thread_setup_wait(struct list_head *wq)
{
	struct list_head *rq;
	struct thread *t;

	rq = cpu_get_ready_queue();
	if (list_is_empty(rq))
		return;

	t = cpu_get_curr_thread();
	assert(t->state == THREAD_STATE_RUNNING);
	t->state = THREAD_STATE_WAITING;
	list_add_tail(wq, &t->wait_entry);
}

// Called at IPL_SCHED
void thread_wait()
{
	struct thread *curr, *next, *idle;
	struct list_head *rq, *e;
	struct thread *thread_switch(struct thread *curr, struct thread *next);

	curr = cpu_get_curr_thread();
	idle = cpu_get_idle_thread();

	rq = cpu_get_ready_queue();
	if (list_is_empty(rq))
		return;

	e = list_del_head(rq);
	next = list_entry(e, struct thread, wait_entry);

	if (next == idle && !list_is_empty(rq)) {
		list_add_tail(rq, &next->wait_entry);
		e = list_del_head(rq);
		next = list_entry(e, struct thread, wait_entry);
	}

	// TODO update ticks for next if required.

	if (curr->state == THREAD_STATE_RUNNING) {
		curr->state = THREAD_STATE_READY;
		list_add_tail(rq, &curr->wait_entry);
	}
	curr = thread_switch(curr, next);
	cpu_set_curr_thread(curr);
}
