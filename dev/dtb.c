// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

#include <lib/string.h>

#include <sys/bits.h>
#include <sys/endian.h>
#include <sys/err.h>

#include "dtb.h"

int dtb_init(struct dtb *dtb, const void *buf)
{
	int i;
	uint32_t *words;

	dtb->buf = buf;
	dtb->hdr = *(const struct dtb_header *)buf;
	words = (uint32_t *)&dtb->hdr;

	for (i = 0; i < 10; ++i)
		words[i] = be32_to_cpu(words[i]);

	if (dtb->hdr.magic != DTB_MAGIC) return ERR_BAD_FILE;

	dtb->num_words = dtb->hdr.size >> 2;
	return ERR_SUCCESS;
}

int dtb_size(const struct dtb *dtb)
{
	if (dtb == NULL) return ERR_PARAM;
	return dtb->hdr.size;
}

static
int dtbn_skip_name(const struct dtb *dtb, int node)
{
	uint32_t i;
	const char *buf;

	buf = (const char *)dtb->buf;

	// Skip until a null char is read.
	for (i = node << 2; buf[i] && i < dtb->hdr.size; ++i) {}

	if (i >= dtb->hdr.size) return ERR_BAD_FILE;

	i = align_up(i + 1, 4);
	return i >> 2;
}

static
int dtbn_skip_props(const struct dtb *dtb, int node)
{
	uint32_t val, size;

	for (;;) {
		if (node >= dtb->num_words) return ERR_BAD_FILE;

		val = be32_to_cpu(dtb->buf[node]);
		if (val != DTB_PROP) break;

		// DTB_PROP, size, name_off.
		if (node + 3 >= dtb->num_words) return ERR_BAD_FILE;

		size = be32_to_cpu(dtb->buf[node + 1]);
		size = align_up(size, 4);
		size >>= 2;

		// Skip DTB_PROP word, size-of-prop-value word,
		// prop-name word, and size-of-prop.

		node += size + 3;
	}
	return node;
}

static
int dtbn_first_child(const struct dtb *dtb, int node)
{
	uint32_t val;
	int prop, child;

	val = be32_to_cpu(dtb->buf[node]);
	if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;

	// Skip over the DTB_BEGIN_NODE to go to the node's name.
	++node;

	// Skip over the name of the node to go to the node's props.
	prop = dtbn_skip_name(dtb, node);
	if (prop < 0) return prop;

	// Skip over the properties of the node.
	child = dtbn_skip_props(dtb, prop);
	if (child < 0) return child;

	// child should point to DTB_END_NODE if there are no children,
	// or to DTB_BEGIN_NODE of the first child.

	val = be32_to_cpu(dtb->buf[child]);

	if (val == DTB_END_NODE) return ERR_NOT_FOUND;
	else if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;
	else return child;
}

static
int dtbn_skip(const struct dtb *dtb, int node)
{
	uint32_t val;
	int depth, prop;

	val = be32_to_cpu(dtb->buf[node]);
	if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;

	depth = 0;
	for (;;) {
		val = be32_to_cpu(dtb->buf[node]);
		++node;
		if (val == DTB_END_NODE) {
			--depth;
			if (depth == 0)	break;
			continue;
		}

		if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;
		++depth;

		prop = dtbn_skip_name(dtb, node);
		if (prop < 0) return prop;

		node = dtbn_skip_props(dtb, prop);
		if (node < 0) return node;
	}
	return node;
}

int dtbn_child(const struct dtb *dtb, int node, const char *name, int len)
{
	int child;
	uint32_t val;

	child = dtbn_first_child(dtb, node);
	if (child < 0) return child;

	for (;;) {
		// child should be the DTB_BEGIN_NODE of the child.
		val = be32_to_cpu(dtb->buf[child]);
		if (val != DTB_BEGIN_NODE) return ERR_NOT_FOUND;

		if (!strncmp(name, (const char *)&dtb->buf[child + 1], len))
			break;

		// Skip to the current child's next sibling.
		child = dtbn_skip(dtb, child);
		if (child < 0) break;

		// If it isn't a DTB_BEGIN_NODE, end the search.
		val = be32_to_cpu(dtb->buf[child]);
		if (val != DTB_BEGIN_NODE) return ERR_NOT_FOUND;
	}
	return child;
}

// Returns the word pointing to DTB_BEGIN_NODE of the requested node.
int dtb_node(const struct dtb *dtb, const char *path)
{
	uint32_t val;
	int i, pos, node;

	if (path == NULL || path[0] != '/') return ERR_PARAM;

	// Root node.
	node = dtb->hdr.off_struct >> 2;
	val = be32_to_cpu(dtb->buf[node]);
	if (val != DTB_BEGIN_NODE) return ERR_BAD_FILE;

	pos = 1;
	for (;;) {
		// Find either a / or a null char.
		for (i = pos; path[i] && path[i] != '/'; ++i) {}

		if (i == pos) {
			if (path[i] == 0) return node;
			return ERR_PARAM;
		}

		node = dtbn_child(dtb, node, &path[pos], i - pos);
		if (node < 0) return node;

		// Skip over the /
		if (path[i] == '/') ++i;
		pos = i;
	}
	return node;
}

int dtbn_prop_read(const struct dtb *dtb, int node, const char *name,
		   void *buf, size_t size)
{
	const char *s;
	struct dtb_prop prop;
	uint32_t val, prop_size;

	if (node < 0 || name == NULL || buf == NULL || size == 0)
		return ERR_PARAM;

	// node must point to DTB_BEGIN_NODE
	val = be32_to_cpu(dtb->buf[node]);
	if (val != DTB_BEGIN_NODE) return ERR_PARAM;

	++node;

	node = dtbn_skip_name(dtb, node);
	if (node < 0) return node;

	for (;;) {
		if (node >= dtb->num_words) return ERR_BAD_FILE;

		val = be32_to_cpu(dtb->buf[node]);
		if (val != DTB_PROP) return ERR_NOT_FOUND;

		prop.len = be32_to_cpu(dtb->buf[node + 1]);
		prop.off_name = be32_to_cpu(dtb->buf[node + 2]);

		s = (const char *)dtb->buf;
		s += dtb->hdr.off_strings + prop.off_name;
		if (!strcmp(s, name)) break;

		prop_size = prop.len;
		prop_size = align_up(prop_size, 4);
		prop_size >>= 2;

		// Skip DTB_PROP word, size-of-prop-value word,
		// prop-name word, and size-of-prop.

		node += prop_size + 3;
	}

	// Found it.

	if (size < prop.len) return ERR_INSUFF_BUFFER;

	// TODO check bounds.
	memcpy(buf, &dtb->buf[node + 3], prop.len);
	return ERR_SUCCESS;
}

int dtb_mem_res_read(const struct dtb *dtb, uint64_t *addr, uint64_t *size,
		     int len)
{
	const char *p;
	int i, num_entries;
	const struct dtb_res_entry *res;

	if (dtb == NULL || addr == NULL || size == NULL || len <= 0)
		return ERR_PARAM;

	p = (const char *)dtb->buf + dtb->hdr.off_mem_res_map;

	num_entries = 0;
	res = (const struct dtb_res_entry *)p;
	for (i = 0; ; ++i, ++res, ++num_entries)
		if (res->size == 0) break;
	if (len < num_entries) return ERR_INSUFF_BUFFER;

	res = (const struct dtb_res_entry *)p;
	for (i = 0; i < num_entries; ++i, ++res) {
		addr[i] = be64_to_cpu(res->addr);
		size[i] = be64_to_cpu(res->size);
	}
	if (i < len) size[i] = 0;

	return ERR_SUCCESS;
}
