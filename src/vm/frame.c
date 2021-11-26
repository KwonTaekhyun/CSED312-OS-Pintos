#include "frame.h"


void frame_init()
{
    list_init(&frame_table);
    clock_hand = list_tail(&frame_table);
}
struct frame *frame_allocate(enum palloc_flags flags)
{
    struct frame *f;
    void *page = palloc_get_page(flags);
    if(page == NULL)
    {
        
    }
}
void frame_deallocate(void *addr)
{
    
}
struct frame *frame_evict()
{
    
}