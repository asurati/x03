// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_SPINLOCK_H
#define SYS_SPINLOCK_H

#include <lib/assert.h>

#include <sys/cpu.h>			// cpu_raise_ipl

struct spin_lock {
	int				lock;
	enum ipl			lock_ipl;
	enum ipl			prev_ipl;
	reg_t				irq_mask;
};

static inline
void spin_lock_init(struct spin_lock *lock, enum ipl lock_ipl)
{
	assert(lock);
	lock->lock_ipl = lock_ipl;
	lock->lock = 0;
}

// Called at ipl less than or equal (in privilege) lock_ipl
static inline
void spin_lock(struct spin_lock *lock)
{
	assert(lock);
	lock->prev_ipl = cpu_raise_ipl(lock->lock_ipl, &lock->irq_mask);
	*(volatile int *)&lock->lock = 1;
	dmb();
}

// Called at ipl == lock_ipl.
static inline
void spin_unlock(struct spin_lock *lock)
{
	assert(lock);
	dmb();
	*(volatile int *)&lock->lock = 0;
	cpu_lower_ipl(lock->prev_ipl, lock->irq_mask);
}

void	spin_lock_init(struct spin_lock *lock, enum ipl lock_ipl);
void	spin_lock(struct spin_lock *lock);
void	spin_unlock(struct spin_lock *lock);
#endif
