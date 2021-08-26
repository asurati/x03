// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/mutex.h>
#include <sys/spinlock.h>

void mutex_init(struct mutex *m)
{
	assert(m);
	m->next = NULL;
	m->lock = 0;
	spin_lock_init(&m->state_lock, IPL_SCHED);
	list_init(&m->wait_queue);
}

// Called at ipl == IPL_THREAD.
void mutex_lock(struct mutex *m)
{
	enum ipl ipl;
	struct thread *curr_thread;
	reg_t irq_mask;

	assert(m);

	curr_thread = cpu_get_curr_thread();
	for (;;) {
		// Disable preemption.
		ipl = cpu_raise_ipl(IPL_SCHED, &irq_mask);

		// We must previously be at IPL_THREAD.
		assert(ipl == IPL_THREAD);

		spin_lock(&m->state_lock);
		// Lock is available, and we are next in line.
		if (m->lock == 0 &&
		    (m->next == NULL || m->next == curr_thread)) {
			m->next = NULL;
			m->lock = 1;
			spin_unlock(&m->state_lock);
			cpu_lower_ipl(ipl, irq_mask);
			break;
		}

		thread_setup_wait(&m->wait_queue);
		spin_unlock(&m->state_lock);
		thread_wait();

		// Enable preemption. Lower IPL to IPL_THREAD, to give
		// IPL_SCHED handlers a chance to run.
		cpu_lower_ipl(ipl, irq_mask);
	}
}

// Called at ipl == IPL_SCHED or IPL_THREAD.
void mutex_unlock(struct mutex *m)
{
	enum ipl ipl;
	struct list_head *e;
	reg_t irq_mask;

	assert(m);

	ipl = cpu_raise_ipl(IPL_SCHED, &irq_mask);
	assert(ipl == IPL_SCHED || ipl == IPL_THREAD);
	spin_lock(&m->state_lock);

	assert(m->lock);
	assert(m->next == NULL);

	if (!list_is_empty(&m->wait_queue)) {
		e = list_del_head(&m->wait_queue);
		m->next = list_entry(e, struct thread, wait_entry);
	}
	m->lock = 0;
	spin_unlock(&m->state_lock);
	if (m->next)
		thread_unwait(m->next);
	cpu_lower_ipl(ipl, irq_mask);
}
