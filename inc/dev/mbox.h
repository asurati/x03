// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_MBOX_H
#define DEV_MBOX_H

int	mbox_get_fw_rev(int *out);
int	mbox_set_dom_state(int dom, int is_on);
#endif
