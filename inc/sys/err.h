// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_ERR_H
#define SYS_ERR_H

enum errors {
	ERR_BAD_FILE		= -0x7fff,
	ERR_NOT_FOUND		= -0x7ffe,
	ERR_PARAM		= -0x7ffd,
	ERR_INSUFF_BUFFER	= -0x7ffc,
	ERR_UNEXP		= -0x7ffb,
	ERR_INVALID		= -0x7ffa,
	ERR_NO_MEM		= -0x7ff9,
	ERR_UNSUP		= -0x7ff8,
	ERR_FAILED		= -0x7ff7,
	ERR_TIMEOUT		= -0x7ff6,
	ERR_PENDING		= -0x7ff5,
	ERR_SUCCESS		= 0,
};
#endif
