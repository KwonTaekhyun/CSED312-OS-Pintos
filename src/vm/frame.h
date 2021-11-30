#include "lib/kernel/list.h"
#include "threads/palloc.h"

struct frame
{
	void *paddr;
	struct pte *pte;
	struct thread *thread;
	struct list_elem elem;
};

struct list frame_table;
struct list_elem *clock_hand;

static struct lock frame_lock;

struct frame *frame_allocate(enum palloc_flags flags);
void frame_deallocate(void *addr);