#include "frame.h"


void frame_init()
{
    list_init(&frame_table);
    clock_hand = list_tail(&frame_table);
}
struct frame *frame_allocate(enum palloc_flags flags)
{
    struct frame *f;
	void *addr;
	if((flags & PAL_USER) == 0)
		return NULL;
	/* allocate physical memory */
	addr = palloc_get_page(flags);
	/* if fail, free physical memory and retry physical memory allocate*/
	while(addr == NULL)
	{
		//try_to_free_pages();
		addr = palloc_get_page(flags);
	}
	f = malloc(sizeof(struct frame));
	if(f == NULL)
	{
		palloc_free_page(addr);
		return NULL;
	}
	/* initialize page */
	f->addr  = addr;
	f->thread = thread_current();
	/* insert page to lru list */
	//add_page_to_lru_list(f);
	return f;
}
void frame_deallocate(void *addr)
{
    
}
struct frame *frame_evict()
{
    
}