// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/list.h>

#include <dev/dtb.h>
#include <dev/fdt.h>

#include "intc.h"
#include "timer.h"
#include "uart.h"
#include "rng.h"
#include "mbox.h"
#include "v3d.h"

#if 0
struct device {
	struct list_head		entry;
	struct list_head		child_head;

	struct device			*bus;
	int				num_addr_cells;
	int				num_size_cells;
};

static struct device g_root_bus, g_simple_bus;
#endif

int dev_init(const struct fdt_node *fdt)
{
	int err;

	err = fdtn_is_compatible(fdt, "brcm,bcm2835");
	if (err) return err;

	// For now, hardcode various values. Later, decide to utilize the fdt
	// in order to form a device tree.

	err = intc_init();
	if (err) return err;

	err = timer_init();
	if (err) return err;

	err = uart_init();
	if (err) return err;

	err = rng_init();
	if (err) return err;

	err = mbox_init();
	if (err) return err;

	// Enable the irqs.
	cpu_enable_irq();

	// v3d_init needs an operational mbox that in turn needs IRQs to be
	// enabled.

	err = v3d_init();
	if (err) goto err0;

	// Keep the IRQs enabled from here on.
	return err;
err0:
	cpu_disable_irq();
	return err;
}
