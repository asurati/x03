// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int	printf(const char *fmt, ...);
int	snprintf(char *str, size_t size, const char *fmt, ...);
int	vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
#endif
