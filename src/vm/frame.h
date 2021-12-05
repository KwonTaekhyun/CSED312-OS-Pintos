#ifndef FRAME_H
#define FRAME_H
#include "lib/kernel/list.h"
#include "threads/palloc.h"
//p3
#include "vm/page.h"

struct frame
{
	void *addr;
	struct pte *pte;
	struct list_elem elem;
};

struct list frame_table;
struct list_elem *clock_hand;

struct frame *frame_allocate(enum palloc_flags flags, struct pte *pte);
void frame_deallocate(void *addr);
struct frame *frame_evict(enum palloc_flags flags);
struct frame *clock_forwarding();
struct lock frame_lock;
#endif