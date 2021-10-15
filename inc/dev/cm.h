// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_CM_H
#define DEV_CM_H

#include <sys/bits.h>

#define CM_PASSWORD			0x5a000000

#define CM_PLLH				(0x110 >> 2)
#define CM_LOCK				(0x114 >> 2)
#define A2W_PLLH_ANA0			(0x1070 >> 2)
#define A2W_PLLH_CTRL			(0x1160 >> 2)
#define A2W_XOCS_CTRL			(0x1190 >> 2)
#define A2W_PLLH_FRAC			(0x1260 >> 2)
#define A2W_PLLH_PIX			(0x1560 >> 2)

#define CM_LOCK_FLOCKH_POS		12
#define CM_LOCK_FLOCKH_BITS		1

#define A2W_PLL_CTRL_PWRDN_POS		16
#define A2W_PLL_CTRL_PRST_DISABLE_POS	17
#define A2W_PLL_CTRL_PWRDN_BITS		1
#define A2W_PLL_CTRL_PRST_DISABLE_BITS	1

#define CM_PLL_ANA_RESET_POS		8
#define CM_PLL_ANA_RESET_BITS		1

#define CM_PLLH_LOAD_PIX_POS		0
#define CM_PLLH_LOAD_PIX_BITS		1

#define A2W_PLL_CHANNEL_DISABLE_POS	8
#define A2W_PLL_CHANNEL_DISABLE_BITS	1

#define A2W_XOCS_CTRL_PLLC_EN_POS	0
#define A2W_XOCS_CTRL_PLLC_EN_BITS	1

#define A2W_PLL_CTRL_NDIV_POS		0
#define A2W_PLL_CTRL_PDIV_POS		12
#define A2W_PLL_CTRL_NDIV_BITS		10
#define A2W_PLL_CTRL_PDIV_BITS		3

#define A2W_PLL_DIV_POS			0
#define A2W_PLL_DIV_BITS		8
#endif
