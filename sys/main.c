// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/vmm.h>

#include <dev/con.h>
#include <dev/mbox.h>
#include <dev/v3d.h>

int kmain()
{
	int err, rev;
	pa_t fb_base;
	size_t fb_size;
	va_t sys_end;
	extern char _sys_end;
	int	pmm_init(va_t *sys_end);
	int	vmm_init(va_t *sys_end);
	int	mmu_init(va_t *sys_end);
	int	slabs_init(va_t *sys_end);

	int	pmm_post_init(va_t sys_end);
	int	mmu_post_init(va_t sys_end);
	int	intc_init();
	int	tmr_init();
	int	mbox_init();
	int	disp_init();
	int	disp_config();
	int	fb_init();
	int	v3d_init();
	int	demo_run();

	sys_end = (va_t)&_sys_end;
	sys_end = align_up(sys_end, 3);

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

	err = tmr_init();
	if (err)
		return err;

	err = mbox_init();
	if (err)
		return err;

	err = mbox_get_fw_rev(&rev);
	if (err)
		return err;
	con_out("rev %x", rev);

	err = disp_init();
	if (err)
		return err;

	err = disp_config();
	if (err)
		return err;
	goto err;

	err = fb_init();
	if (err)
		return err;

	err = v3d_init();
	if (err)
		return err;

	err = demo_run();
	if (err)
		return err;
err:
	return err;
	(void)fb_base;
	(void)fb_size;
}
