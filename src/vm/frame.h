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
	struct thread *thread;
	struct list_elem elem;
};

struct list frame_table;
struct list_elem *clock_hand;
struct frame *frame_allocate(enum palloc_flags flags);
void frame_deallocate(void *addr);

#endif