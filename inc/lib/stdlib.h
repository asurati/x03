// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef LIB_STDLIB_H
#define LIB_STDLIB_H

#include <stddef.h>

void	*malloc(size_t size);
void	free(void *p);
void	srand(unsigned int seed);
int	rand();
#endif
