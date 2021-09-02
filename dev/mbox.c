// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/vmm.h>
#include <sys/cpu.h>

#include <dev/con.h>

int	slabs_va_to_pa(void *va, pa_t *pa);

struct mbox_tag {
	uint32_t			id;
	uint32_t			buf_size;
	uint32_t			code;
};

struct mbox_msg {
	uint32_t			size;
	uint32_t			code;
};

struct mbox_tag_get_fw_rev {
	struct mbox_tag			tag;
	struct {
		uint32_t		rev;
	} buf;
};

struct mbox_tag_set_dom_state {
	struct mbox_tag			tag;
	struct {
		uint32_t		dom;
		int			is_on;
	} buf;
};

struct mbox_tag_fb_set_depth {
	struct mbox_tag			tag;
	struct {
		uint32_t		bpp;
	} buf;
};

struct mbox_tag_fb_set_dim {
	struct mbox_tag			tag;
	struct {
		uint32_t		width;
		uint32_t		height;
	} buf;
};

struct mbox_tag_fb_alloc {
	struct mbox_tag			tag;
	struct {
		uint32_t		base;
		uint32_t		size;
	} buf;
};

struct mbox_tag_fb_free {
	struct mbox_tag			tag;
	struct {
		uint32_t		base;
	} buf;
};

struct mbox {
	uint32_t			rw;
	uint32_t			res[3];
	uint32_t			peek;
	uint32_t			sender;
	uint32_t			status;
	uint32_t			config;
};

static volatile struct mbox *g_mbox;

static
void mbox_send(struct mbox_msg *m, pa_t pa)
{
	dc_civac(m, m->size);
	dsb();

	for (;;) {
		if (g_mbox[1].status & 0x80000000ul)
			continue;
		break;
	}
	g_mbox[1].rw = pa_to_ba(pa);
}

static
int mbox_check_tag(const struct mbox_tag *t)
{
	size_t size;
	if (!(t->code & 0x80000000ul))
		return ERR_FAILED;
	size = t->code & 0x7ffffffful;
	if (size != t->buf_size)
		return ERR_FAILED;
	return ERR_SUCCESS;
}

static
int mbox_recv(struct mbox_msg *m, pa_t pa)
{
	int err;
	uint32_t val;
	struct mbox_tag *t;

	for (;;) {
		if (g_mbox[0].status & 0x40000000ul)
			continue;
		break;
	}

	val = g_mbox[0].rw;
	if (val != pa_to_ba(pa))
		return ERR_FAILED;
	if (m->code != 0x80000000ul)
		return ERR_FAILED;

	t = (struct mbox_tag *)(m + 1);
	for (;t->id;) {
		err = mbox_check_tag(t);
		if (err)
			return err;
		t = (struct mbox_tag *)((char *)(t + 1) + t->buf_size);
	}
	return ERR_SUCCESS;
}

// IPL_THREAD
int mbox_free_fb(pa_t base)
{
	int err;
	size_t size;
	struct mbox_tag_fb_free *t;
	struct mbox_msg *m;
	pa_t pa;

	size = sizeof(*t) + sizeof(*m) + sizeof(uint32_t);
	size = align_up(size, 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		goto err0;

	memset(m, 0, size);
	m->size = size;

	t = (struct mbox_tag_fb_free *)(m + 1);

	t->tag.id = 0x48001;
	t->tag.buf_size = sizeof(t->buf);
	t->buf.base = base;

	mbox_send(m, pa | 8);
	err = mbox_recv(m, pa | 8);
err0:
	free(m);
	return err;
}

// IPL_THREAD
int mbox_alloc_fb(pa_t *out_base, size_t *out_size)
{
	int err;
	size_t size;
	struct mbox_tag_fb_set_depth *td;
	struct mbox_tag_fb_set_dim *tp, *tv;
	struct mbox_tag_fb_alloc *ta;
	struct mbox_msg *m;
	pa_t pa;

	size = sizeof(*td) + sizeof (*tp) + sizeof(*tv) + sizeof(*ta);
	size += sizeof(*m) + sizeof(uint32_t);
	size = align_up(size, 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		goto err0;

	memset(m, 0, size);
	m->size = size;

	tp = (struct mbox_tag_fb_set_dim *)(m + 1);
	tv = (struct mbox_tag_fb_set_dim *)(tp + 1);
	td = (struct mbox_tag_fb_set_depth *)(tv + 1);
	ta = (struct mbox_tag_fb_alloc *)(td + 1);

	tp->tag.buf_size = sizeof(tp->buf);
	tv->tag.buf_size = sizeof(tv->buf);
	td->tag.buf_size = sizeof(td->buf);
	ta->tag.buf_size = sizeof(ta->buf);

	tp->tag.id = 0x48003;
	tp->buf.width = 1280;
	tp->buf.height = 720;

	*tv = *tp;
	tv->tag.id = 0x48004;

	td->tag.id = 0x48005;
	td->buf.bpp = 32;

	ta->tag.id = 0x40001;
	ta->buf.base = PAGE_SIZE;

	mbox_send(m, pa | 8);
	err = mbox_recv(m, pa | 8);
	if (err)
		goto err0;
	*out_base = ta->buf.base;
	*out_size = ta->buf.size;
err0:
	free(m);
	return err;
}

// IPL_THREAD
int mbox_set_dom_state(int dom, int is_on)
{
	int err;
	size_t size;
	struct mbox_tag_set_dom_state *t;
	struct mbox_msg *m;
	pa_t pa;

	is_on = !!is_on;

	size = sizeof(*t) + sizeof(*m) + sizeof(uint32_t);
	size = align_up(size, 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		goto err0;

	memset(m, 0, size);
	m->size = size;

	t = (struct mbox_tag_set_dom_state *)(m + 1);
	t->tag.id = 0x38030;
	t->tag.buf_size = sizeof(t->buf);
	t->buf.dom = dom;
	t->buf.is_on = is_on;

	mbox_send(m, pa | 8);
	err = mbox_recv(m, pa | 8);
	if (err)
		goto err0;
	if (t->buf.is_on != is_on)
		err = ERR_FAILED;
err0:
	free(m);
	return err;
}

// IPL_THREAD
int mbox_get_fw_rev(int *out)
{
	int err;
	size_t size;
	struct mbox_tag_get_fw_rev *t;
	struct mbox_msg *m;
	pa_t pa;

	size = sizeof(*t) + sizeof(*m) + sizeof(uint32_t);
	size = align_up(size, 16);
	m = malloc(size);
	if (m == NULL)
		return ERR_NO_MEM;
	err = slabs_va_to_pa(m, &pa);
	if (err)
		goto err0;

	memset(m, 0, size);
	m->size = size;

	t = (struct mbox_tag_get_fw_rev *)(m + 1);
	t->tag.id = 1;
	t->tag.buf_size = sizeof(t->buf);

	mbox_send(m, pa | 8);
	err = mbox_recv(m, pa | 8);
	if (err)
		goto err0;
	*out = t->buf.rev;
err0:
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
	g_mbox = (volatile struct mbox *)va;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
