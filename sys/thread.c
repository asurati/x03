// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/pmm.h>
#include <sys/vmm.h>
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

// IPL_THREAD
int thread_create(fn_thread *fn, void *p, struct thread **out)
{
	int err;
	struct thread *t;
	vpn_t page;
	pfn_t frame;
	va_t va;
	reg_t mask;
	enum ipl prev_ipl;
	struct list_head *rq;
	void	thread_enter();

	err = ERR_NO_MEM;
	t = malloc(sizeof(*t));
	if (t == NULL)
		goto err0;

	err = pmm_alloc(ALIGN_PAGE, 1, &frame);
	if (err)
		goto err1;

	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err2;

	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW);
	if (err)
		goto err3;

	va = vpn_to_va(page);
	va += PAGE_SIZE;
	t->regs[0] = va;			// sp
	t->regs[1] = (va_t)thread_enter;	// lr
	t->regs[2] = (va_t)fn;			// r4
	t->regs[3] = (va_t)p;			// r5
	t->state = THREAD_STATE_READY;

	prev_ipl = cpu_raise_ipl(IPL_SCHED, &mask);
	rq = cpu_get_ready_queue();
	list_add_tail(rq, &t->wait_entry);
	cpu_lower_ipl(prev_ipl, mask);
	*out = t;
	return ERR_SUCCESS;
err3:
	vmm_free(page, 1);
err2:
	pmm_free(frame, 1);
err1:
	free(t);
err0:
	return err;
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
