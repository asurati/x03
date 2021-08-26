// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/condvar.h>

void cond_var_init(struct cond_var *v)
{
	assert(v);
	spin_lock_init(&v->state_lock, IPL_SCHED);
	list_init(&v->wait_queue);
	v->signalled = 0;
	v->num_waiters = 0;
}

// Called at ipl == IPL_THREAD.
void cond_var_wait(struct cond_var *v, struct mutex *lock)
{
	enum ipl ipl;
	reg_t irq_mask;

	assert(v);

	for (;;) {
		ipl = cpu_raise_ipl(IPL_SCHED, &irq_mask);
		assert(ipl == IPL_THREAD);
		spin_lock(&v->state_lock);

		if (!v->signalled) {
			++v->num_waiters;
			thread_setup_wait(&v->wait_queue);
			spin_unlock(&v->state_lock);
			mutex_unlock(lock);
			thread_wait();
			cpu_lower_ipl(ipl, irq_mask);
			mutex_lock(lock);
		} else {
			if (v->num_waiters == 0)
				v->signalled = 0;
			spin_unlock(&v->state_lock);
			cpu_lower_ipl(ipl, irq_mask);
			break;
		}
	}
}

// Called at ipl == IPL_SCHED or IPL_THREAD.
void cond_var_signal(struct cond_var *v)
{
	enum ipl ipl;
	struct list_head wq, *e;
	struct thread *t;
	reg_t irq_mask;

	assert(v);

	list_init(&wq);

	ipl = cpu_raise_ipl(IPL_SCHED, &irq_mask);
	assert(ipl == IPL_SCHED || ipl == IPL_THREAD);

	spin_lock(&v->state_lock);
	if (!list_is_empty(&v->wait_queue)) {
		list_add_head(&v->wait_queue, &wq);
		list_del_entry(&v->wait_queue);
		list_init(&v->wait_queue);
		v->signalled = 1;
		assert(v->num_waiters);
	}

	list_for_each(e, &wq) {
		t = list_entry(e, struct thread, wait_entry);
		thread_unwait(t);
		--v->num_waiters;
	}
	spin_unlock(&v->state_lock);
	cpu_lower_ipl(ipl, irq_mask);
}
