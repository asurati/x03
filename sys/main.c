// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/thread.h>

#include <dev/dtb.h>
#include <dev/fdt.h>
#include <dev/timer.h>
#include <dev/uart.h>
#include <dev/mbox.h>
#include <dev/v3d.h>

extern int heap_init(pa_t *krnl_end);
extern int dev_init(const struct fdt_node *fdt);
extern int demo_run();

// By default, main is placed in .text.startup. Our setup needs it to not be
// in .text.startup.

__attribute__((section(".text")))
void main(unsigned long dtb_addr)
{
	pa_t krnl_end;
	extern char _krnl_end;
	int err;
	static struct dtb dtb;
	const struct fdt_node *fdt = NULL;

	krnl_end = (pa_t)&_krnl_end;

	err = cpu_init();
	if (err) goto err0;

	err = thread_init();
	if (err) goto err0;

	err = dtb_init(&dtb, (const void *)dtb_addr);
	if (err) goto err0;

	err = mmu_init(&dtb);
	if (err) goto err0;

	err = pmm_init(&dtb, &krnl_end);
	if (err) goto err0;

	err = heap_init(&krnl_end);
	if (err) goto err0;

	err = pmm_post_init(&dtb, krnl_end);
	if (err) goto err0;

	err = fdt_init((const void *)dtb_addr, &fdt);
	if (err) goto err0;

	err = dev_init(fdt);
	fdtn_free((struct fdt_node *)fdt);
	if (err) goto err0;

	//err = demo_run();
	if (err) goto err0;
	return;
err0:
	for(;;) wfi();
}
