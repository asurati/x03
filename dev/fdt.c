// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

#include <lib/assert.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#include <dev/dtb.h>

#include <sys/bits.h>
#include <sys/endian.h>
#include <sys/err.h>
#include <sys/list.h>

#include "dtb.h"

struct fdt_node {
	char				*name;
	struct list_head		entry;
	struct list_head		prop_head;
	struct list_head		child_head;
};

struct fdt_prop {
	char				*name;
	union {
		char			*val;
		char			buf[sizeof(char *)];
	} u;
	struct list_head		entry;
	size_t				size;
};

static
void fdtp_free(struct fdt_prop *prop)
{
	if (prop == NULL) return;
	if (prop->name) free(prop->name);
	if (prop->size > sizeof(char *) && prop->u.val) free(prop->u.val);
	free(prop);
}

void fdtn_free(struct fdt_node *node)
{
	struct fdt_node *child;
	struct list_head *e, *head;
	struct fdt_prop *prop;

	if (node == NULL) return;
	if (node->name) free(node->name);

	head = &node->prop_head;
	while (!list_is_empty(head)) {
		e = list_del_head(head);
		prop = list_entry(e, struct fdt_prop, entry);
		fdtp_free(prop);
	}

	head = &node->child_head;
	while (!list_is_empty(head)) {
		e = list_del_head(head);
		child = list_entry(e, struct fdt_node, entry);
		fdtn_free(child);
	}
	free(node);
}

// _node points to DTB_PROP.
static
int fdt_prop(const struct dtb *dtb, int _node, struct fdt_prop **out,
	     int *next)
{
	const char *name;
	int n, err, size, len;
	struct dtb_prop dtb_prop;
	struct fdt_prop *prop;

	err = ERR_NO_MEM;
	n = _node;
	size = sizeof(*prop);
	prop = malloc(size);
	if (prop == NULL) goto error;

	memset(prop, 0, size);

	// Get the in-file property.
	dtb_prop.len = be32_to_cpu(dtb->buf[n + 1]);
	dtb_prop.off_name = be32_to_cpu(dtb->buf[n + 2]);

	// Allocate the name.
	name = (const char *)dtb->buf;
	name += dtb->hdr.off_strings + dtb_prop.off_name;

	err = ERR_INVALID;
	len = strlen(name);
	assert(len > 0);
	if (len <= 0) goto error;

	err = ERR_NO_MEM;
	prop->name = malloc(len + 1);
	if (prop->name == NULL) goto error;

	strcpy(prop->name, name);
	prop->size = dtb_prop.len;

	// If the value is sufficiently small, store in-place.
	if (prop->size <= sizeof(char *)) {
		memcpy(&prop->u.buf[0], &dtb->buf[n + 3], prop->size);
	} else {
		prop->u.val = malloc(prop->size);
		if (prop->u.val == NULL) goto error;
		memcpy(prop->u.val, &dtb->buf[n + 3], prop->size);
	}

	// Align size to the next 4-byte boundary.
	size = align_up(prop->size, 4);

	// Skip DTB_PROP word, size-of-prop-value word, name-off word,
	// value and any padding.

	n += 3 + (size >> 2);
	*out = prop;
	*next = n;
	return ERR_SUCCESS;
error:
	fdtp_free(prop);
	return err;
}

// _node points to DTB_BEGIN_NODE.
static
int fdt_node(const struct dtb *dtb, int _node, struct fdt_node **out,
	     int *next)
{
	uint32_t val;
	const char *name;
	int len, err, n, lnext;
	struct fdt_node *node, *child;
	struct fdt_prop *prop;

	if (_node < 0 || dtb == NULL || out == NULL)
		return ERR_PARAM;

	n = _node;

	val = be32_to_cpu(dtb->buf[n]);
	if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;

	err = ERR_NO_MEM;
	node = malloc(sizeof(*node));
	if (node == NULL) goto error;

	memset(node, 0, sizeof(*node));
	list_init(&node->prop_head);
	list_init(&node->child_head);

	// Skip DTB_BEGIN_NODE.
	++n;

	// Skip to the node's name.
	name = (const char *)dtb->buf;
	name += n << 2;

	// Allocate space for node's name.
	len = strlen(name);
	if (len) {
		node->name = malloc(len + 1);
		if (node->name == NULL) goto error;
		strcpy(node->name, name);
	}

	// Skip to the property's node. TODO Overflow.
	n = (n << 2) + (len + 1);	// +1 for the null char.
	n = align_up(n, 4) >> 2;

	assert(n >= 0);

	for (;;) {
		err = ERR_BAD_FILE;
		if (n >= dtb->num_words) goto error;

		val = be32_to_cpu(dtb->buf[n]);
		if (val != DTB_PROP) break;

		// DTB_PROP, size, name_off.
		if (n + 3 >= dtb->num_words) goto error;

		prop = NULL;
		err = fdt_prop(dtb, n, &prop, &lnext);
		if (err) goto error;
		list_add_tail(&node->prop_head, &prop->entry);
		n = lnext;
	}

	// Done with the properties. Now parse child-nodes.
	for (;;) {
		err = ERR_BAD_FILE;
		if (n >= dtb->num_words) goto error;

		val = be32_to_cpu(dtb->buf[n]);
		if (val == DTB_END_NODE) {
			// At the end of this node.
			++n;
			break;
		}

		// If this isn't the end of this node and this isn't the
		// beginning of the child node, then this is either a bad file
		// or an unsupported construct.

		if (val == DTB_NOP) {
			// Skip NOP.
			++n;
			continue;
		}

		if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;

		child = NULL;
		err = fdt_node(dtb, n, &child, &lnext);
		if (err) goto error;
		list_add_tail(&node->child_head, &child->entry);
		n = lnext;
	}
	*out = node;
	if (next)
		*next = n;
	return ERR_SUCCESS;
error:
	fdtn_free(node);
	return err;
}

int fdt_init(const void *buf, const struct fdt_node **_out)
{
	int err, node;
	static struct dtb dtb;
	struct fdt_node *out;

	// buf is allowed to be NULL, since RAM_BASE is 0.
	if (_out == NULL)
		return ERR_PARAM;

	err = dtb_init(&dtb, buf);
	if (err) return err;

	node = dtb_node(&dtb, "/");
	if (node < 0) return node;

	out = NULL;
	err = fdt_node(&dtb, node, &out, NULL);
	if (err) return err;
	*_out = out;
	return ERR_SUCCESS;
}

static
int fdtn_find_prop(const struct fdt_node *node, const char *name,
		   const struct fdt_prop **out)
{
	struct list_head *e;
	struct fdt_prop *prop;

	// out is optional.
	if (node == NULL || name == NULL)
		return ERR_PARAM;

	list_for_each(e, &node->prop_head) {
		prop = list_entry(e, struct fdt_prop, entry);
		if (strcmp(prop->name, name))
			continue;
		if (out) *out = prop;
		return ERR_SUCCESS;
	}
	return ERR_NOT_FOUND;
}

static
int fdtp_size(const struct fdt_prop *prop, size_t *out)
{
	if (prop == NULL || out == NULL)
		return ERR_PARAM;
	*out = prop->size;
	return ERR_SUCCESS;
}

static
int fdtp_read(const struct fdt_prop *prop, void *buf, size_t *size)

{
	if (prop == NULL || buf == NULL || size == NULL || size == 0)
		return ERR_PARAM;

	if (*size < prop->size) {
		*size = prop->size;
		return ERR_INSUFF_BUFFER;
	}

	*size = prop->size;
	if (prop->size <= sizeof(char *))
		memcpy(buf, &prop->u.buf[0], prop->size);
	else
		memcpy(buf, prop->u.val, prop->size);
	return ERR_SUCCESS;
}

int fdtn_read_prop(const struct fdt_node *node, const char *name, void *buf,
		   size_t *size)
{
	int err;
	const struct fdt_prop *prop;

	if (node == NULL || name == NULL || buf == NULL ||
	    size == NULL || *size == 0)
		return ERR_PARAM;

	prop = NULL;
	err = fdtn_find_prop(node, name, &prop);
	if (err) return err;
	return fdtp_read(prop, buf, size);
}

int fdtn_find_child_p(const struct fdt_node *node, const char *prop_name,
		      const struct fdt_node **out)
{
	int err;
	struct list_head *e;
	struct fdt_node *child;

	if (node == NULL || prop_name == NULL || out == NULL)
		return ERR_PARAM;

	list_for_each(e, &node->child_head) {
		child = list_entry(e, struct fdt_node, entry);
		err = fdtn_find_prop(child, prop_name, NULL);
		if (!err) {
			*out = child;
			return err;
		} else if (err != ERR_NOT_FOUND) {
			return err;
		}
	}
	return ERR_NOT_FOUND;
}

int fdtn_is_compatible(const struct fdt_node *node, const char *name)
{
	int err;
	size_t size, i;
	const char *buf;
	const struct fdt_prop *prop;

	if (node == NULL || name == NULL)
		return ERR_PARAM;

	prop = NULL;
	size = 0;

	err = fdtn_find_prop(node, "compatible", &prop);
	if (err) return err;
	err = fdtp_size(prop, &size);
	if (err) return err;

	if (prop->size <= sizeof(char *))
		buf = &prop->u.buf[0];
	else
		buf = prop->u.val;

	for (i = 0; i < size; ++i) {
		if (!strcmp(&buf[i], name))
			return ERR_SUCCESS;
		i += strlen(&buf[i]);
		assert(buf[i] == 0);
	}
	return ERR_NOT_FOUND;
}
