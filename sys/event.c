// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/event.h>
#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/thread.h>

#define EVENT_SIGNALLED			1

void event_init(struct event *e)
{
	e->value = 0;
	list_init(&e->wq);
}

// Should only be called at IPL_THREAD.
void event_wait(struct event *e)
{
	// The function shouldn't raise the IPL once and then continue
	// looping at the raised IPL. That prevents soft IRQs and other
	// components running at IPL_SOFT from running; the routines that
	// set the event may run at IPL_SOFT.

	for (;;) {
		cpu_disable_preemption();
		if (e->value == EVENT_SIGNALLED) {
			cpu_enable_preemption();
			break;
		}
		thread_wait_and_switch(&e->wq);

		// This lowering of IPL provides an opportunity to run the
		// soft irqs.

		cpu_enable_preemption();
	}
}

// Should only be called at IPL_SOFT.
void event_reset_preempt_disabled(struct event *e)
{
	assert(list_is_empty(&e->wq));
	e->value = 0;
}

// Should only be called at IPL_THREAD.
void event_reset(struct event *e)
{
	cpu_disable_preemption();
	event_reset_preempt_disabled(e);
	cpu_enable_preemption();
}

// Should only be called at IPL_SOFT.
void event_set_preempt_disabled(struct event *e)
{
	e->value = EVENT_SIGNALLED;
	thread_wakeup_all(&e->wq);
}

// Should only be called at IPL_THREAD.
void event_set(struct event *e)
{
	cpu_disable_preemption();
	event_set_preempt_disabled(e);
	cpu_enable_preemption();
}
