// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/vmm.h>
#include <sys/spinlock.h>

#include <dev/ioq.h>

#define MBOX_TAG_GET_FW_REV		1ul
#define MBOX_TAG_SET_DOM_STATE		0x38030ul

int	slabs_va_to_pa(void *va, pa_t *pa);

struct mbox_msg {
	uint32_t			size;
	uint32_t			code;
};

struct mbox_msg_tag {
	uint32_t			id;
	uint32_t			buf_size;
	uint32_t			data_size;
};

struct mbox_msg_get_fw_rev {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		uint32_t		rev;
	} buf;
	uint32_t			end_tag;
};

struct mbox_msg_set_dom_state {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		uint32_t		dom;
		int			is_on;
	} buf;
	uint32_t			end_tag;
};

struct mbox_regs {
	uint32_t			v2a_read;
	uint32_t			res0[3];
	uint32_t			v2a_peek;
	uint32_t			v2a_sender;
	uint32_t			v2a_status;
	uint32_t			v2a_config;
	uint32_t			a2v_write;
	uint32_t			res1[5];
	uint32_t			a2v_status;
};

#define MBOX_V2ACFG_DIRQ_ENABLE_POS	0
#define MBOX_V2ACFG_DIRQ_PENDING_POS	4
#define MBOX_V2ACFG_DIRQ_ENABLE_BITS	1
#define MBOX_V2ACFG_DIRQ_PENDING_BITS	1

static volatile struct mbox_regs *g_mbox_regs;
static struct ioq g_mbox_ioq;

// IPL_HARD
static
void mbox_hw_irqh()
{
	uint32_t val;

	val = g_mbox_regs->v2a_config;
	if (!bits_get(val, MBOX_V2ACFG_DIRQ_PENDING))
		return;

	// Clear the signal.
	g_mbox_regs->v2a_read;

	cpu_raise_sw_irq(IRQ_ARM_MAILBOX);
}

// IPL_SCHED
static
void mbox_sw_irqh()
{
	ioq_complete_ior(&g_mbox_ioq);
}

// IPL_THREAD, or IPL_SOFT
static
int mbox_ioq_req(struct ior *ior)
{
	pa_t pa;
	struct mbox_msg *m;

	m = ior_param(ior);
	pa = ior_param_pa(ior);
	pa = pa_to_ba(pa) | 8;	// Channel: ArmReq, VCRes
	dc_civac(m, m->size);
	dsb();

	for (;;) {
		if (g_mbox_regs->a2v_status & 0x80000000ul)
			continue;
		break;
	}
	g_mbox_regs->a2v_write = pa;
	return ERR_SUCCESS;
}

// IPL_SOFT
static
int mbox_ioq_res(struct ior *ior)
{
	int err;
	struct mbox_msg *m;
	struct mbox_msg_tag *t;
	size_t size;

	m = ior_param(ior);
	err = ERR_FAILED;
	if (m->code != 0x80000000ul)
		goto err0;

	t = (struct mbox_msg_tag *)(m + 1);
	size = t->data_size & 0x7ffffffful;
	if ((t->data_size & 0x80000000ul) && size == t->buf_size)
		err = ERR_SUCCESS;
err0:
	return err;
}

// IPL_THREAD
int mbox_set_dom_state(int dom, int is_on)
{
	int err;
	size_t size;
	struct ior ior;
	struct mbox_msg_set_dom_state *m;
	pa_t pa;

	size = align_up(sizeof(*m), 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		return err;

	is_on = !!is_on;
	m->msg.size = size;
	m->msg.code = 0;
	m->msg_tag.id = MBOX_TAG_SET_DOM_STATE;
	m->msg_tag.buf_size = sizeof(m->buf);
	m->msg_tag.data_size = 0;
	m->buf.dom = dom;
	m->buf.is_on = is_on;
	m->end_tag = 0;

	ior_init(&ior, &g_mbox_ioq, 0, m, pa);
	err = ioq_queue_ior(&ior);
	if (!err)
		err = ior_wait(&ior);
	if (!err && m->buf.is_on != is_on)
		err = ERR_FAILED;
	free(m);
	return err;
}

// IPL_THREAD
int mbox_get_fw_rev(int *out)
{
	int err;
	size_t size;
	struct ior ior;
	struct mbox_msg_get_fw_rev *m;
	pa_t pa;

	size = align_up(sizeof(*m), 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		return err;

	m->msg.size = size;
	m->msg.code = 0;
	m->msg_tag.id = MBOX_TAG_GET_FW_REV;
	m->msg_tag.buf_size = sizeof(m->buf);
	m->msg_tag.data_size = 0;
	m->end_tag = 0;

	ior_init(&ior, &g_mbox_ioq, 0, m, pa);
	err = ioq_queue_ior(&ior);
	if (!err)
		err = ior_wait(&ior);
	if (!err)
		*out = m->buf.rev;
	free(m);
	return err;
}

// IPL_THREAD
int mbox_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;
	uint32_t val;

	ioq_init(&g_mbox_ioq, mbox_ioq_req, mbox_ioq_res);

	pa = MBOX_BASE;
	frame = pa_to_pfn(pa);
	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va += pa & (PAGE_SIZE - 1);
	g_mbox_regs = (volatile struct mbox_regs *)va;
	cpu_register_irqh(IRQ_ARM_MAILBOX, mbox_hw_irqh, mbox_sw_irqh);

	// ARM_MC_IHAVEDATAIRQEN
	val = g_mbox_regs->v2a_config;
	val |= bits_on(MBOX_V2ACFG_DIRQ_ENABLE);
	g_mbox_regs->v2a_config = val;
	cpu_enable_irq(IRQ_ARM_MAILBOX);
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
