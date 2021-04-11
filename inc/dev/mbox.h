// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_MBOX_H
#define DEV_MBOX_H

enum rpi_domain {
	RPI_DOMAIN_VC_3D = 10,
};

int	mbox_get_clock_rate(int clock_id, unsigned int *out);
int	mbox_get_firmware_revision(unsigned int *out);
int	mbox_get_board_revision(unsigned int *out);
int	mbox_get_vc_memory(unsigned int *base, unsigned int *size);
int	mbox_enable_disable_qpu(int is_enable);
int	mbox_set_domain_state(enum rpi_domain domain, unsigned int on);
#endif
