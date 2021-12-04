#include "frame.h"
#include "threads/thread.h"
#include "lib/debug.h"
#include "userprog/pagedir.h"


void frame_init()
{
    list_init(&frame_table);
    clock_hand = NULL;
}
struct frame *frame_allocate(enum palloc_flags flags, struct pte *pte)
{   
    struct frame *frame;

    // palloc_get_page()를 통해 페이지 할당
    void *page = palloc_get_page(flags);
    if(!page){
        frame = frame_evict(flags);
    }
    else{
        frame = malloc(sizeof(struct frame));
        if(!frame) {
            palloc_free_page(page);
            return NULL;
        }
        else{
            frame->addr = page;
            frame->pte = pte;
        }
    }

    pte->frame = frame;

    // frame_table에 frame 추가
    list_push_back(&(frame_table), &(frame->elem));

    // frame 주소 반환
    return frame;
}
void frame_deallocate(void *addr)
{
	// 물리 주소 addr에 해당하는 frame 구조체를 frame_table에서 검색
    struct thread *current_thread = thread_current();
    struct list_elem *e;
    struct frame *frame_entry = NULL;
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e)){
        struct frame *frame = list_entry (e, struct frame, elem);
        if (frame->addr == addr){
            frame_entry = frame;
            break;
        }
    }
    if(!frame_entry){
        // deallocate할 frame이 존재하지 않음
        return;
    }

    // frame을 frame_table에서 제거
    list_remove(&(frame_entry->elem));

	palloc_free_page(frame_entry->addr);

    free(frame_entry);
}
struct frame *frame_evict(enum palloc_flags flags)
{
    // clock algorithm을 통해 evict할 frame을 선택
    struct frame *frame = clock_forwarding();

    int n = list_size(&frame_table) * 2;

    while(n-- > 0){
        if(frame->pte->pinned){
            frame = clock_forwarding();
            continue;
        }
        else if(pagedir_is_accessed(thread_current()->pagedir, frame->pte->vaddr)){
            pagedir_set_accessed (thread_current()->pagedir, frame->pte->vaddr, false);
            frame = clock_forwarding();
            continue;
        }
        break;
    }

    // P3-6-test
    printf("find evict frame!\n");

    // eviction
    bool is_dirty = pagedir_is_dirty(frame->pte->thread->pagedir, frame->pte->vaddr)
      || pagedir_is_dirty(frame->pte->thread->pagedir, frame->addr);

	if(frame->pte->type == VM_FILE){
		mmap_file_write_at(frame->pte->file, frame->addr, frame->pte->read_bytes, frame->pte->offset);
	}
	else{
		frame_swap_out(frame);
	}

    // frame 정보 및 데이터 제거 후 frame_table에서 제거
	frame->pte->is_loaded = false;
    frame->pte->frame = NULL;
    pagedir_clear_page(frame->pte->thread->pagedir, frame->pte->vaddr);
    list_remove(&(frame->elem));

    // page 새로 할당 후 frame 초기화
    void *page = palloc_get_page(flags);
    frame->addr = page;
    frame->pte = NULL;

    return frame;
}

struct frame *clock_forwarding(){
    clock_hand = (clock_hand == NULL || clock_hand == list_end(&frame_table)) ? 
        list_begin(&frame_table) : list_next(&clock_hand);

    return list_entry(clock_hand, struct frame, elem);
}

void frame_swap_out(struct frame *frame){
    size_t swap_index = swap_out(frame->addr);

    frame->pte->swap_index = swap_index;
    frame->pte->type = VM_ANON;
}