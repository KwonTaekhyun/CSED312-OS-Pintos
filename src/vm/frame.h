#include "lib/kernel/list.h"

struct frame
{
	void *addr;
	struct pte *pte;
	struct thread *thread;
	struct list_elem elem;
};

struct list frame_table;
struct list_elem *clock_hand;