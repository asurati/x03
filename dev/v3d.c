// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdio.h>
#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/mbox.h>
#include <dev/uart.h>

#include "intc.h"
#include "v3d.h"

// Should be called at IPL_HARD only.
static
void v3d_irqh(void *p)
{
	unsigned long intctl, dbqitc;
	static char buf[512];

	intctl = readl(V3D_INTCTL);
	dbqitc = readl(V3D_DBQITC);
	snprintf(buf, sizeof(buf), "v3dirqh %x %x\r\n", intctl, dbqitc);
	uart_send_string(buf);

	intctl &= 0xf;
	dbqitc &= bits_on(V3D_DBQITC_ALL);

	// Deassert the signals.
	if (intctl) writel(V3D_INTCTL, intctl);
	if (dbqitc) writel(V3D_DBQITC, dbqitc);

	// Raise the soft irq.
	if (intctl || dbqitc)
		cpu_raise_soft_irq(IRQ_VC_3D);
	(void)p;
}

// Should be called at IPL_SOFT only.
static
void v3d_soft_irqh(void *p)
{
	(void)p;
}

int v3d_init()
{
	int err;
	unsigned int domain;
	unsigned long val;

	domain = RPI_DOMAIN_VC_3D + 1;
	err = mbox_set_domain_state(domain, 1);
	if (err) return err;

	if (readl(V3D_IDENT0) != V3D_IDENT0_VAL) {
		mbox_set_domain_state(domain, 0);
		return ERR_NOT_FOUND;
	}
	cpu_register_irqh(IRQ_VC_3D, v3d_irqh, NULL);
	cpu_register_soft_irqh(IRQ_VC_3D, v3d_soft_irqh, NULL);

	// Allow QPU host_interrupts to reach the host.
	val = bits_on(V3D_DBCFG_QITENA);
	writel(V3D_DBCFG, val);

	// Enable QPU host_interrupts for all QPUs.
	val = bits_on(V3D_DBQITE_ALL);
	writel(V3D_DBQITE, val);

	// Deassert any stale host_interrupts.
	val = bits_on(V3D_DBQITC_ALL);
	writel(V3D_DBQITC, val);

	writel(V3D_INTDIS, 0xf);

	// Enable the IRQ.
	intc_enable_irq(IRQ_VC_3D);
	return ERR_SUCCESS;
}
