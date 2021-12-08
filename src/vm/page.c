#include "vm/page.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "lib/string.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

#include "lib/kernel/bitmap.h"

bool pt_init(struct hash *pt)
{
    return hash_init(pt, pt_hash_func, pt_less_func, NULL);
}
static unsigned pt_hash_func(const struct hash_elem *e, void *aux)
{
    struct pte *page = hash_entry(e,struct pte, elem);
    return hash_int(page->vaddr);
}
static bool pt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
    struct pte *page_a = hash_entry(a,struct pte, elem);
    struct pte *page_b = hash_entry(b,struct pte, elem);
    if(page_a->vaddr < page_b->vaddr) return true;
    else return false;
}
bool pte_insert(struct hash *pt, struct pte *pte)
{
    struct hash_elem *ret;
    ret = hash_insert(pt, &pte->elem);
    if (ret==NULL) return true;
    else return false;
}
bool pte_delete(struct hash *pt, struct pte *pte)
{
    struct hash_elem *ret;
    ret = hash_delete(pt, &pte->elem);
    free(pte);
    if (ret==NULL) return false;
    else return true;
}
struct pte *pte_find(void *vaddr)
{
    struct pte page;
    struct hash_elem *e;
    page.vaddr = pg_round_down(vaddr);
    e = hash_find(&thread_current()->page_table, &page.elem);
    if(e==NULL) return NULL;
    else return hash_entry(e,struct pte, elem);
}
void pt_destroy(struct hash *pt)
{
    if(pt!=NULL) hash_destroy(pt, pt_destroy_func);
}
void pt_destroy_func(struct hash_elem *e, void *aux)
{
    struct pte *page;
    page = hash_entry(e,struct pte, elem);
    void *addr;
    if(page->is_loaded)
    {
        addr = pagedir_get_page(thread_current()->pagedir,page->vaddr);
        frame_deallocate(addr);
        pagedir_clear_page(thread_current()->pagedir, page->vaddr);
    }
    pte_delete(&thread_current()->page_table, page);
} 
bool load_file(struct frame *frame, struct pte *p)
{
    if(file_read_at(p->file, frame->addr, p->read_bytes, p->offset)!=(p->read_bytes))
    {
        //frame_deallocate(frame->addr);
        return false;
    }
    memset((frame->addr)+(p->read_bytes), 0, p->zero_bytes);
    return true;
}

bool pte_create_by_file(void* addr, struct file* file, off_t offset, size_t read_bytes, size_t zero_bytes, bool writable)
{
    if(pte_find(addr) != NULL){
        return false;
    }
    struct pte* page_entry = malloc(sizeof(struct pte));
    if(page_entry != NULL)
    {
        page_entry->vaddr = addr;
        page_entry->file = file;

        page_entry->type = VM_FILE;

        page_entry->offset = offset;
        page_entry->read_bytes = read_bytes;
        page_entry->zero_bytes = zero_bytes;

        page_entry->is_loaded = false;

        page_entry->writable = writable;

        page_entry->frame = NULL;
        page_entry->swap_index = BITMAP_ERROR;
        page_entry->thread = thread_current();

        page_entry->pinned = false;

        return pte_insert(&(thread_current()->page_table), page_entry);
    }
    else
    {
        return false;
    }
}