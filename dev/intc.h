// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef L_DEV_INTC_H
#define L_DEV_INTC_H

#include <sys/cpu.h>

int	intc_init();
void	intc_disable_irq(enum irq irq);
void	intc_enable_irq(enum irq irq);
#endif
