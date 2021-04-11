// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/mmu.h>

#define THREAD_STACK_SIZE		4096
#define THREAD_STATE_RUNNING		1
#define THREAD_STATE_READY		2
#define THREAD_STATE_WAITING		3

#define THREAD_DEFAULT_TICKS		10

struct thread_context {
	unsigned long			padding;
	unsigned long			cpsr;
	unsigned long			reg[13];
	unsigned long			lr;
};

struct thread {
	struct thread_context		*context;
	struct list_head		entry;
	int				ticks;
	int				state;
	int				padding;
};

extern void _thread_switch(struct thread_context **curr_context,
			   struct thread_context **next_context);
static struct list_head g_ready;

// Referred to in crt0.S
char g_idle_thread_stack[THREAD_STACK_SIZE]
__attribute__((aligned(THREAD_STACK_SIZE))) = {0};

static struct thread *g_idle_thread =
(struct thread *)(&g_idle_thread_stack[THREAD_STACK_SIZE] -
		  sizeof(struct thread));

int thread_init()
{
	list_init(&g_ready);
	g_idle_thread->context = NULL;
	g_idle_thread->ticks = 0;
	g_idle_thread->state = THREAD_STATE_RUNNING;
	return ERR_SUCCESS;
}

__attribute__((noreturn))
void thread_idle()
{
	for (;;) wfi();
}

void thread_begin(fn_thread *fn, void *p)
{
	cpu_enable_preemption();
	fn(p);
}

// Should only be called at IPL_SOFT.
void thread_switch()
{
	struct thread *curr, *next;
	struct list_head *e;

	curr = thread_get_current();

	// We call switch only if there's a thread available to switch to.
	assert(!list_is_empty(&g_ready));

	// Remove a ready thread.
	e = list_del_head(&g_ready);
	next = container_of(e, struct thread, entry);

	// If we picked the idle thread, and there's more available, queue the
	// the idle thread back, and pick another.

	if (next == g_idle_thread && !list_is_empty(&g_ready)) {
		list_add_tail(&g_ready, &next->entry);
		e = list_del_head(&g_ready);
		next = container_of(e, struct thread, entry);
	}

	if (next != g_idle_thread && next->ticks <= 0)
		next->ticks = THREAD_DEFAULT_TICKS;

	// The current thread may have been placed on a wait queue. In that
	// case, we don't touch its state and linkage.

	if (curr->state == THREAD_STATE_RUNNING) {
		curr->state = THREAD_STATE_READY;
		list_add_tail(&g_ready, &curr->entry);
	}
	_thread_switch(&curr->context, &next->context);
}

// Should only be called at IPL_SOFT.
void thread_wait_and_switch(struct list_head *wq)
{
	struct thread *t;

	// No choice but to return, since there are no other threads available.
	if (list_is_empty(&g_ready))
		return;

	t = thread_get_current();
	assert(t->state == THREAD_STATE_RUNNING);
	t->state = THREAD_STATE_WAITING;
	list_add_tail(wq, &t->entry);
	thread_switch();
}

// Should only be called at IPL_SOFT.
struct thread *thread_wakeup_one(struct list_head *wq)
{
	struct thread *t;
	struct list_head *e;

	e = list_del_head(wq);
	t = container_of(e, struct thread, entry);
	assert(t->state == THREAD_STATE_WAITING);
	t->state = THREAD_STATE_READY;
	list_add_tail(&g_ready, &t->entry);
	return t;
}

// Should only be called at IPL_SOFT.
void thread_wakeup_all(struct list_head *wq)
{
	while (!list_is_empty(wq))
		thread_wakeup_one(wq);
}

struct thread *thread_create(fn_thread *fn, void *p)
{
	struct thread *t;
	char *stack;
	size_t off;

	stack = malloc(THREAD_STACK_SIZE);
	if (stack == NULL) return NULL;

	off = THREAD_STACK_SIZE - sizeof(struct thread);
	t = (struct thread *)(stack + off);
	off -= sizeof(struct thread_context);
	t->context = (struct thread_context *)(stack + off);
	t->context->lr = (pa_t)thread_begin;
	t->context->reg[0] = (pa_t)fn;
	t->context->reg[1] = (pa_t)p;
	t->context->cpsr = mrs_cpsr();
	t->state = THREAD_STATE_READY;
	cpu_disable_preemption();
	list_add_tail(&g_ready, &t->entry);
	cpu_enable_preemption();
	return t;
}
