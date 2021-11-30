#include "frame.h"


void frame_init()
{
    list_init(&frame_table);
    clock_hand = list_head(&frame_table);

    lock_init (&frame_lock);
}
struct frame *frame_allocate(enum palloc_flags flags)
{
    lock_acquire (&frame_lock);

    void *page = palloc_get_page(flags);
    if(!page)
    {
        // page allocation failed.
        /* first, swap out the page */
        // clear the page mapping, and replace it with swap
    }

    struct frame *frame = malloc(sizeof(struct frame));
    if(!frame) {
        lock_release (&frame_lock);
        return NULL;
    }

    frame->paddr = page;
    frame->thread = thread_current ();

    lock_release (&frame_lock);
    return frame;
}

void frame_deallocate(void *addr)
{
    lock_acquire (&frame_lock);
    
    lock_release (&frame_lock);
}
struct frame *frame_evict()
{
    
}