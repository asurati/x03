// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_LIST_H
#define SYS_LIST_H

#include <stddef.h>

#ifndef container_of
#define container_of(p, t, m)		(t *)((char *)p - offsetof(t, m))
#endif

struct list_head {
	struct list_head		*prev;
	struct list_head		*next;
};

#define list_entry(p, t, m)		container_of(p, t, m)
#define list_for_each(pos, head)					\
	for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

static inline
void list_init(struct list_head *head)
{
	head->next = head->prev = head;
}

static inline
int list_is_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline
void list_add_between(struct list_head *prev, struct list_head *next,
		      struct list_head *n)
{
	n->next = next;
	n->prev = prev;
	next->prev = n;
	prev->next = n;
}

static inline
void list_add_head(struct list_head *head, struct list_head *n)
{
	list_add_between(head, head->next, n);
}

static inline
void list_add_tail(struct list_head *head, struct list_head *n)
{
	list_add_between(head->prev, head, n);
}

static inline
void list_del_entry(struct list_head *entry)
{
	struct list_head *prev, *next;
	next = entry->next;
	prev = entry->prev;
	next->prev = prev;
	prev->next = next;
}

static inline
struct list_head *list_del_tail(struct list_head *head)
{
	struct list_head *entry = head->prev;
	list_del_entry(entry);
	return entry;
}

static inline
struct list_head *list_del_head(struct list_head *head)
{
	struct list_head *entry = head->next;
	list_del_entry(entry);
	return entry;
}

static inline
struct list_head *list_peek_tail(struct list_head *head)
{
	struct list_head *entry = head->prev;
	return entry;
}

static inline
struct list_head *list_peek_head(struct list_head *head)
{
	struct list_head *entry = head->next;
	return entry;
}

#endif
