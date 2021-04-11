// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stddef.h>

int strncmp(const char *a, const char *b, size_t len)
{
	size_t i;
	for (i = 0; i < len && a[i] && b[i] && a[i] == b[i]; ++i) {}
	if (i == len) return 0;
	return a[i] - b[i];
}

int strlen(const char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {}
	return i;
}

int strcmp(const char *a, const char *b)
{
	size_t i;
	for (i = 0; a[i] && b[i] && a[i] == b[i]; ++i) {}
	return a[i] - b[i];
}

char *strcpy(char *dst, const char *src)
{
	size_t i;
	for (i = 0; src[i]; ++i)
		dst[i] = src[i];
	dst[i] = 0;
	return dst;
}

void *memcpy(void *d, const void *s, size_t n)
{
	size_t i;
	char *dst = d;
	const char *src = s;
	for (i = 0; i < n; ++i)
		dst[i] = src[i];
	return d;
}

void *memset(void *s, int c, size_t n)
{
	size_t i;
	char *str = s;
	for (i = 0; i < n; ++i)
		str[i] = c;
	return s;
}
