// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef LIB_STRING_H
#define LIB_STRING_H

#include <stddef.h>

int	strncmp(const char *a, const char *b, size_t len);
int	strcmp(const char *a, const char *b);
int	strlen(const char *a);

char	*strcpy(char *dst, const char *src);
void	*memset(void *s, int c, size_t n);
void	*memcpy(void *d, const void *s, size_t n);
#endif
