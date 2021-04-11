// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_ERR_H
#define SYS_ERR_H

enum errors {
	ERR_BAD_FILE = -0x7fff,
	ERR_NOT_FOUND,
	ERR_PARAM,
	ERR_INSUFF_BUFFER,
	ERR_UNEXP,
	ERR_INVALID,
	ERR_NO_MEM,
	ERR_UNSUP,
	ERR_FAILED,
	ERR_TIMEOUT,
	ERR_SUCCESS = 0
};
#endif
