// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdarg.h>

#include <lib/assert.h>
#include <lib/stdio.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/mmu.h>
#include <sys/vmm.h>
#include <sys/spinlock.h>

#define CON_BUF_LEN			256

#define CON_DR				(0 >> 2)
#define CON_FR				(0x18 >> 2)

static struct spin_lock g_con_lock;
static int g_con_init;
static volatile uint32_t *g_con_regs;

void _con_out(const char *str, int len)
{
	int i;

	for (i = 0; i < len; ++i) {
		// Loop until there's space in Tx FIFO.
		while (g_con_regs[CON_FR] & 0x20)
			;
		g_con_regs[CON_DR] = str[i];
	}
}

// IPL_THREAD
int con_out(const char *fmt, ...)
{
	int len;
	va_list ap;
	static char g_con_buf[CON_BUF_LEN];

	if (!g_con_init)
		return ERR_UNSUP;

	spin_lock(&g_con_lock);
	va_start(ap, fmt);
	len = vsnprintf(g_con_buf, CON_BUF_LEN, fmt, ap);
	va_end(ap);

	if (len < 0 || len >= CON_BUF_LEN || g_con_buf[len])
		return ERR_INSUFF_BUFFER;

	// Append a \n. For Minicom, enable addcarreturn option.
	if (len == CON_BUF_LEN - 1)
		len -= 1;
	g_con_buf[len] = '\n';
	g_con_buf[len + 1] = 0;
	len += 1;

	_con_out(g_con_buf, len);
	spin_unlock(&g_con_lock);
	return len;
}

// IPL_THREAD
int con_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	spin_lock_init(&g_con_lock, IPL_HARD);

	pa = UART_BASE;
	frame = pa_to_pfn(pa);
	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va += pa & (PAGE_SIZE - 1);
	g_con_regs = (volatile uint32_t *)va;
	g_con_init = 1;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
