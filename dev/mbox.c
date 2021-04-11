// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/mmu.h>

#include "intc.h"
#include "mbox.h"
#include "ioq.h"

#define MBOX_V2A_READ			(void *)(IO_BASE + 0xb880)
#define MBOX_V2A_PEEK			(void *)(IO_BASE + 0xb890)
#define MBOX_V2A_SENDER			(void *)(IO_BASE + 0xb894)
#define MBOX_V2A_STATUS			(void *)(IO_BASE + 0xb898)
#define MBOX_V2A_CONFIG			(void *)(IO_BASE + 0xb89c)

#define MBOX_V2A_CFG_DIRQ_ENABLE_POS	0
#define MBOX_V2A_CFG_DIRQ_PENDING_POS	4
#define MBOX_V2A_CFG_DIRQ_ENABLE_BITS	1
#define MBOX_V2A_CFG_DIRQ_PENDING_BITS	1

#define MBOX_A2V_WRITE			(void *)(IO_BASE + 0xb8a0)
#define MBOX_A2V_STATUS			(void *)(IO_BASE + 0xb8b8)

static struct ioq g_mbox_ioq;

// Should be called at IPL_HARD only.
static
void mbox_irqh(void *p)
{
	unsigned long val;

	val = readl(MBOX_V2A_CONFIG);
	if (!bits_get(val, MBOX_V2A_CFG_DIRQ_PENDING))
		return;

	// Deassert the signal.
	readl(MBOX_V2A_READ);

	// Raise the soft irq.
	cpu_raise_soft_irq(IRQ_MAILBOX);
	(void)p;
}

// Should be called at IPL_SOFT only.
static
void mbox_soft_irqh(void *p)
{
	struct ioq *ioq = (struct ioq *)p;
	ioq_complete_curr_ior(ioq);
}

// Should be called at IPL_SOFT only.
void mbox_ioq_request(struct ior *ior)
{
	struct mbox_msg *msg;
	pa_t pa;
	unsigned long val;

	msg = ior_param(ior);
	pa = pa_to_bus((pa_t)msg);
	pa |= 8;		// Channel. Arm requestor, VC responder.

	// Clean and Invalidate the data cache of the msg buffer.
	dc_civac(msg, msg->size);
	dsb();

	for (;;) {
		val = readl(MBOX_A2V_STATUS);
		if (val & 0x80000000ul)
			continue;
		break;
	}
	writel(MBOX_A2V_WRITE, pa);
}

// Should be called at IPL_SOFT only.
void mbox_ioq_response(struct ior *ior)
{
	int ret;
	struct mbox_msg *msg;
	struct mbox_msg_tag *tag;
	size_t size;

	ret = ERR_FAILED;
	msg = ior_param(ior);

	if (msg->code != 0x80000000u)
		goto done;

	tag = (struct mbox_msg_tag *)(msg + 1);
	if ((tag->data_size & 0x80000000u) == 0)
		goto done;

	size = tag->data_size & (~0x80000000u);
	if (size != tag->buf_size)
		goto done;
	ret = ERR_SUCCESS;
done:
	ior_signal(ior, ret);
}

// Should be called at IPL_THREAD only.
int mbox_get_clock_rate(int clock_id, unsigned int *out)
{
	int err;
	size_t size;
	struct mbox_msg_get_clock_rate *msg;
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_GET_CLOCK_RATE;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->buf.clock_id = clock_id;
	msg->end_tag = 0;

	// There's a single command, so 0 suffices.
	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	if (!err)
		*out = msg->buf.clock_rate;
	free(msg);
	return err;
}

// Should be called at IPL_THREAD only.
int mbox_get_board_revision(unsigned int *out)
{
	int err;
	size_t size;
	struct mbox_msg_get_firmware_revision *msg;	// Similar struct.
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_GET_BOARD_REVISION;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->end_tag = 0;

	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	if (!err)
		*out = msg->buf.rev;
	free(msg);
	return err;
}

// Should be called at IPL_THREAD only.
int mbox_get_firmware_revision(unsigned int *out)
{
	int err;
	size_t size;
	struct mbox_msg_get_firmware_revision *msg;
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_GET_FIRMWARE_REVISION;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->end_tag = 0;

	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	if (!err)
		*out = msg->buf.rev;
	free(msg);
	return err;
}

// Should be called at IPL_THREAD only.
int mbox_get_vc_memory(unsigned int *base, unsigned int *sz)
{
	int err;
	size_t size;
	struct mbox_msg_get_vc_memory *msg;
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_GET_VC_MEMORY;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->end_tag = 0;

	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	if (!err) {
		*base = msg->buf.base;
		*sz = msg->buf.size;
	}
	free(msg);
	return err;
}

// Should be called at IPL_THREAD only.
int mbox_enable_disable_qpu(int is_enable)
{
	int err;
	size_t size;
	struct mbox_msg_enable_qpu *msg;
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_ENABLE_QPU;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->buf.is_enable = !!is_enable;
	msg->end_tag = 0;

	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	free(msg);
	return err;
}

// Should be called at IPL_THREAD only.
int mbox_set_domain_state(enum rpi_domain domain, unsigned int on)
{
	int err;
	size_t size;
	struct mbox_msg_set_domain_state *msg;
	struct ior ior;

	size = sizeof(*msg);
	size = align_up(size, 16);	// 16-byte alignment.
	msg = malloc(size);
	if (msg == NULL) return ERR_NO_MEM;

	on = !!on;
	msg->msg.size = size;
	msg->msg.code = 0;
	msg->msg_tag.id = MBOX_TAG_SET_DOMAIN_STATE;
	msg->msg_tag.buf_size = sizeof(msg->buf);
	msg->msg_tag.data_size = 0;
	msg->buf.domain = domain;
	msg->buf.on = on;
	msg->end_tag = 0;

	ior_init(&ior, 0, msg);
	ioq_queue_ior(&g_mbox_ioq, &ior);
	err = ior_wait(&ior);
	if (!err && msg->buf.on != on) err = ERR_FAILED;
	free(msg);
	return err;
}

int mbox_init()
{
	unsigned long val;

	// The intc ensures that the mbox irq starts as disabled.

	ioq_init(&g_mbox_ioq, mbox_ioq_request, mbox_ioq_response);
	cpu_register_irqh(IRQ_MAILBOX, mbox_irqh, NULL);
	cpu_register_soft_irqh(IRQ_MAILBOX, mbox_soft_irqh, &g_mbox_ioq);

	// Allow mbox irq to be raised when data is available in the
	// VC -> ARM  (from VC to ARM) direction.

	// See ARM_MC_IHAVEDATAIRQEN inside the qemu sources.

	val = readl(MBOX_V2A_CONFIG);
	val |= bits_on(MBOX_V2A_CFG_DIRQ_ENABLE);
	writel(MBOX_V2A_CONFIG, val);

	intc_enable_irq(IRQ_MAILBOX);
	return ERR_SUCCESS;
}
