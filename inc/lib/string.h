// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef LIB_STRING_H
#define LIB_STRING_H

#include <stddef.h>

static inline
int strncmp(const char *a, const char *b, size_t len)
{
	size_t i;

	for (i = 0; i < len && a[i] && b[i] && a[i] == b[i]; ++i)
		;
	if (i == len)
		return 0;
	return a[i] - b[i];
}

static inline
int strlen(const char *s)
{
	size_t i;
	for (i = 0; s[i]; ++i)
		;
	return i;
}

static inline
int strcmp(const char *a, const char *b)
{
	size_t i;

	for (i = 0; a[i] && b[i] && a[i] == b[i]; ++i)
		;
	return a[i] - b[i];
}

static inline
char *strcpy(char *d, const char *s)
{
	size_t i;

	for (i = 0; s[i]; ++i)
		d[i] = s[i];
	d[i] = 0;
	return d;
}

static inline
void *memcpy(void *d, const void *s, size_t n)
{
	size_t i;
	char *dst = d;
	const char *src = s;

	for (i = 0; i < n; ++i)
		dst[i] = src[i];
	return d;
}

static inline
void *memset(void *s, int c, size_t n)
{
	size_t i;
	char *str = s;

	for (i = 0; i < n; ++i)
		str[i] = c;
	return s;
}
#endif
