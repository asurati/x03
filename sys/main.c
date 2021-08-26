// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/slab.h>
#include <sys/vmm.h>

#include <dev/con.h>
#include <dev/mbox.h>

int kmain()
{
	int err, rev;
	va_t sys_end;
	extern char _sys_end;
	int	mmu_init(va_t *sys_end);
	int	mmu_post_init(va_t sys_end);
	int	intc_init();
	int	mbox_init();

	sys_end = (va_t)&_sys_end;
	sys_end = align_up(sys_end, 8);

	err = pmm_init(&sys_end);
	if (err)
		goto err;

	err = vmm_init(&sys_end);
	if (err)
		goto err;

	err = slabs_init(&sys_end);
	if (err)
		goto err;

	err = mmu_init(&sys_end);
	if (err)
		goto err;

	err = pmm_post_init(sys_end);
	if (err)
		goto err;

	// Switches the mmu from loader to ourselves.
	err = mmu_post_init(sys_end);
	if (err)
		goto err;

	// Consoles do not use interrupts yet; they can be initialized
	// before intc is.
	err = con_init();
	if (err)
		goto err;

	err = intc_init();
	if (err)
		return err;

	err = mbox_init();
	if (err)
		return err;

	err = mbox_get_fw_rev(&rev);
	if (err)
		return err;
	con_out("rev %x", rev);
err:
	return err;
}
