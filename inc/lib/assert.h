// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef LIB_ASSERT_H
#define LIB_ASSERT_H

extern void do_assert(const char *msg, const char *func, const char *file,
		      int line);

#if defined(NDEBUG)
#define assert(c)	((void)0)
#else	// NDEBUG
#define assert(c)							\
	do {								\
		if ((c) == 0)						\
		do_assert(#c, __func__, __FILE__, __LINE__);		\
	} while (0)
#endif	// NDEBUG
#endif
