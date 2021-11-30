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

    // palloc_get_page()를 통해 페이지 할당
    void *page = palloc_get_page(flags);
    if(!page)
    {
        // page allocation failed.
        /* first, swap out the page */
        // clear the page mapping, and replace it with swap
        lock_release (&frame_lock);
        return NULL;
    }

    // page 구조체를 할당, 초기화
    struct frame *frame = malloc(sizeof(struct frame));
    if(!frame) {
        lock_release (&frame_lock);
        return NULL;
    }

    frame->paddr = page;
    frame->thread = thread_current ();

    // frame_table에 frame 추가
    list_push_back(&(frame_table), &(frame->elem));

    lock_release (&frame_lock);

    // frame 주소 반환
    return frame;
}

void frame_deallocate(void *addr)
{
    // 물리 주소 addr에 해당하는 frame 구조체를 frame_table에서 검색
    struct thread *current_thread = thread_current();
    struct list_elem *e;
    struct frame *frame_entry;
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e)){
        frame_entry = list_entry (e, struct frame, elem);
        if (frame_entry->paddr == addr){
            break;
        }
    }
    if(!frame_entry){
        // deallocate할 frame이 존재하지 않음
        return;
    }

    lock_acquire (&frame_lock);

    // frame을 frame_table에서 제거
    list_remove(&(frame_entry->elem));

    // frame의 실제 page에 할당한 메모리 해제
    palloc_free_page(addr);
    // frame에 할당한 메모리 해제
    free(frame_entry);
    
    lock_release (&frame_lock);
}
struct frame *frame_evict()
{
    
}