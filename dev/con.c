// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdarg.h>

#include <lib/assert.h>
#include <lib/stdio.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/mmu.h>
#include <sys/vmm.h>
#include <sys/mutex.h>

#define CON_BUF_LEN			256

struct con_regs {
	uint32_t			data;
	uint32_t			res[5];
	uint32_t			flag;
};

static struct mutex g_con_mutex;
static int g_con_init;
static volatile struct con_regs *g_con_regs;

void _con_out(const char *str, int len)
{
	int i;

	for (i = 0; i < len; ++i) {
		// Loop until there's space in Tx FIFO.
		while (g_con_regs->flag & 0x20)
			;
		g_con_regs->data = str[i];
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

	mutex_lock(&g_con_mutex);
	va_start(ap, fmt);
	len = vsnprintf(g_con_buf, CON_BUF_LEN, fmt, ap);
	va_end(ap);
	assert(len >= 0 && len < CON_BUF_LEN);
	assert(g_con_buf[len] == 0);

	// Append a \n. For Minicom, enable addcarreturn option.
	if (len == CON_BUF_LEN - 1)
		len -= 1;
	g_con_buf[len] = '\n';
	g_con_buf[len + 1] = 0;
	len += 1;
	assert(len < CON_BUF_LEN);

	_con_out(g_con_buf, len);
	mutex_unlock(&g_con_mutex);
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

	mutex_init(&g_con_mutex);

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
	g_con_regs = (volatile struct con_regs *)va;
	g_con_init = 1;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
