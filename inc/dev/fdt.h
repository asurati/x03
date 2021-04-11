// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_FDT_H
#define DEV_FDT_H

#include <stddef.h>
#include <stdint.h>

struct	fdt_node;
struct	fdt_prop;

int	fdt_init(const void *buf, const struct fdt_node **out);
void	fdtn_free(struct fdt_node *node);
int	fdtn_read_prop(const struct fdt_node *node, const char *name,
		       void *buf, size_t *size);
int	fdtn_find_child_p(const struct fdt_node *node, const char *prop_name,
			  const struct fdt_node **out);
int	fdtn_is_compatible(const struct fdt_node *node, const char *name);
#endif
