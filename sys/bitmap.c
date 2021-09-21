// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/string.h>

#include <sys/bitmap.h>
#include <sys/bits.h>
#include <sys/err.h>

int bitmap_init(struct bitmap *map, void *buf, int num_bits)
{
	if (map == NULL || buf == NULL || num_bits <= 0)
		return ERR_PARAM;

	map->buf = buf;
	map->num_bits = num_bits;
	memset(buf, 0, num_bits >> 3);
	// Caller ensures proper alignment.

	return ERR_SUCCESS;
}

static
int bitmap_on_off(struct bitmap *map, int start, int num_bits, char is_on)
{
	int i, ix, bit;

	if (map == NULL || start < 0 || num_bits <= 0)
		return ERR_PARAM;
	if (start + num_bits <= 0)
		return ERR_PARAM;
	if (start + num_bits > map->num_bits)
		return ERR_PARAM;

	ix = start >> 6;
	bit = start & 0x3f;
	for (i = 0; i < num_bits; ++i) {
		if (is_on)
			map->buf[ix] |= 1ull << bit;
		else
			map->buf[ix] &= ~(1ull << bit);
		bit = (bit + 1) & 0x3f;
		if (bit == 0)
			++ix;
	}
	return ERR_SUCCESS;
}

static
int bitmap_is_on_off(const struct bitmap *map, int start, int num_bits,
		     char is_on)
{
	uint64_t val;
	int i, ix, bit;

	if (map == NULL || start < 0 || num_bits <= 0)
		return ERR_PARAM;
	if (start + num_bits <= 0)
		return ERR_PARAM;
	if (start + num_bits > map->num_bits)
		return ERR_PARAM;

	ix = start >> 6;
	bit = start & 0x3f;
	for (i = 0; i < num_bits; ++i) {
		val = map->buf[ix] & (1ull << bit);

		if (is_on && val == 0)
			return ERR_UNEXP;
		if (!is_on && val)
			return ERR_UNEXP;

		bit = (bit + 1) & 0x3f;
		if (bit == 0)
			++ix;
	}
	return ERR_SUCCESS;
}

static
int bitmap_find_on_off(const struct bitmap *map, int align, int start,
		       int num_bits, char is_on)
{
	uint64_t val;
	int i, ix, bit, incr;

	if (map == NULL || num_bits <= 0 || num_bits > map->num_bits)
		return ERR_PARAM;

	incr = 1 << align;
	start = align_up(start, align);

	if (start + num_bits <= 0)
		return ERR_PARAM;
	if (start + num_bits > map->num_bits)
		return ERR_PARAM;

	for (; start < map->num_bits; start += incr) {
		ix = start >> 6;
		bit = start & 0x3f;
		for (i = 0; i < num_bits && start + i < map->num_bits; ++i) {
			val = map->buf[ix] & (1ull << bit);

			if (is_on && val == 0)
				break;
			if (!is_on && val)
				break;

			bit = (bit + 1) & 0x3f;
			if (bit == 0)
				++ix;
		}

		if (i == num_bits)
			break;
	}
	if (start < 0)
		return ERR_NOT_FOUND;
	return start;
}

int bitmap_on(struct bitmap *map, int start, int num_bits)
{
	return bitmap_on_off(map, start, num_bits, 1);
}

int bitmap_off(struct bitmap *map, int start, int num_bits)
{
	return bitmap_on_off(map, start, num_bits, 0);
}

int bitmap_is_on(const struct bitmap *map, int start, int num_bits)
{
	return bitmap_is_on_off(map, start, num_bits, 1);
}

int bitmap_is_off(const struct bitmap *map, int start, int num_bits)
{
	return bitmap_is_on_off(map, start, num_bits, 0);
}

int bitmap_find_off(const struct bitmap *map, int align, int start,
		    int num_bits)
{
	return bitmap_find_on_off(map, align, start, num_bits, 0);
}
