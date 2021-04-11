// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_ENDIAN_H
#define SYS_ENDIAN_H

#include <stdint.h>

static inline
uint16_t bswap16(uint16_t v)
{
	return (v << 8) | (v >> 8);
}

static inline
uint32_t bswap32(uint32_t v)
{
	uint32_t lo = bswap16(v >> 16);
	uint32_t hi = bswap16(v);
	return (hi << 16) | lo;
}

static inline
uint64_t bswap64(uint64_t v)
{
	uint64_t lo = bswap32(v >> 32);
	uint64_t hi = bswap32(v);
	return (hi << 32) | lo;
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define be16_to_cpu(x)			bswap16(x)
#define be32_to_cpu(x)			bswap32(x)
#define be64_to_cpu(x)			bswap64(x)
#define cpu_to_be64(x)			bswap64(x)
#else	// __BYTE_ORDER__
#error "Unsupported endian."
#endif	// __BYTE_ORDER__
#endif
