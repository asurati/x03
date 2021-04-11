// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/bits.h>
#include <sys/err.h>
#include <sys/mmu.h>

#define UART_DR				(void *)(IO_BASE + 0x201000)
#define UART_FR				(void *)(IO_BASE + 0x201018)

#define UART_FR_TXFF_POS		5
#define UART_FR_TXFF_BITS		1

int uart_init()
{
	// The intc ensures that the uart irq starts as disabled.

	// There might not be any need to initialize the UART, since
	// qemu happens to work without such initialization, and on HW,
	// the uboot would have already initialized it.

	// We also do not rely on the UART's interrupts yet.

	return ERR_SUCCESS;
}

static
void uart_send(char c)
{
	unsigned long val;

	// Loop while the tx fifo is full.
	for (;;) {
		val = readl(UART_FR);
		if (!bits_get(val, UART_FR_TXFF))
			break;
	}
	writel(UART_DR, c);
}

void uart_send_string(const char *str)
{
	while (*str) {
		uart_send(*str);
		++str;
	}
}
