// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdio.h>

#include <sys/cpu.h>

#include <dev/uart.h>

__attribute__((noreturn))
void do_assert(const char *func, const char *file, int line)
{
	static char buf[512];

	snprintf(buf, sizeof(buf), "asrt: %s,%s,%x\r\n", func, file, line);
	uart_send_string(buf);

	for (;;) wfi();
}
