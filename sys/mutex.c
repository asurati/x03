// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/mutex.h>
#include <sys/thread.h>

#define MUTEX_UNLOCKED			0
#define MUTEX_LOCKED			1
#define MUTEX_UNLOCKING			2

void mutex_init(struct mutex *m)
{
	m->value = MUTEX_UNLOCKED;
	list_init(&m->wq);
}

// Should only be called at IPL_THREAD.
void mutex_lock(struct mutex *m)
{
	struct thread *t;

	// The function shouldn't raise the IPL once and then continue
	// looping at the raised IPL. That prevents soft IRQs and other
	// components running at IPL_SOFT from running; the routines that
	// set the event may run at IPL_SOFT.
	for (;;) {
		cpu_disable_preemption();
		t = thread_get_current();
		if ((m->value == MUTEX_UNLOCKED) ||
		    (m->value == MUTEX_UNLOCKING && m->next == t)) {
			m->next = NULL;
			m->value = MUTEX_LOCKED;
			cpu_enable_preemption();
			break;
		}
		thread_wait_and_switch(&m->wq);

		// This lowering of IPL provides an opportunity to run the
		// soft irqs.

		cpu_enable_preemption();
	}
}

// Should only be called at IPL_SOFT.
void mutex_unlock_preempt_disabled(struct mutex *m)
{
	if (list_is_empty(&m->wq)) {
		m->value = MUTEX_UNLOCKED;
	} else {
		m->value = MUTEX_UNLOCKING;
		m->next = thread_wakeup_one(&m->wq);
	}
}

// Should only be called at IPL_THREAD.
void mutex_unlock(struct mutex *m)
{
	cpu_disable_preemption();
	mutex_unlock_preempt_disabled(m);
	cpu_enable_preemption();
}
