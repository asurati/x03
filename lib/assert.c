// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdio.h>

#include <sys/cpu.h>

#include <dev/con.h>

__attribute__((noreturn))
void do_assert(const char *msg, const char *func, const char *file, int line)
{
	con_out("asrt: \"%s\", %s, %s, %d", msg, func, file, line);

	for (;;)
		cpu_yield();
}
