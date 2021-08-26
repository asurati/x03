// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_BITS_H
#define SYS_BITS_H

// Although these are macros, they are usually used with flags which are also
// defined as macros. To reduce the all-caps noise, keep these in lower-case.

// a must be a power of 2.
#define align_down(v, a)		((v) & ~((a) - 1))
#define align_up(v, a)			(((v) + (a) - 1) & ~((a) - 1))
#define is_aligned(v, a)		(((v) & ((a) - 1)) == 0)

#if defined(__ASSEMBLER__)
#define bits_size(f)			(1 << f##_BITS)
#else
#define bits_size(f)			(1ul << f##_BITS)
#endif
#define bits_mask(f)			(bits_size(f) - 1)
#define bits_set(f, v)			(((v) & bits_mask(f)) << f##_POS)
#define bits_get(v, f)			(((v) >> f##_POS) & bits_mask(f))
#define bits_push(f, v)			((v) & (bits_mask(f) << f##_POS))
#define bits_pull(v, f)			bits_push(f, v)
#define bits_on(f)			(bits_mask(f) << f##_POS)
#define bits_off(f)			~bits_on(f)
#endif
