#include "page.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"

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
    if(page_a->vaddr > page_b->vaddr) return true;
    else return false;
}
bool pte_insert(struct hash *pt, struct pte *pte)
{
    hash_insert(pt, &pte->elem);
}
bool pte_delete(struct hash *pt, struct pte *pte)
{
    hash_delete(pt, &pte->elem);
}
struct pte *pte_find(void *vaddr)
{
    struct pte *page;
    struct hash_elem *e;
    page->vaddr = pg_round_down(vaddr);
    hash_find(&thread_current()->page_table, &page->elem);
    if(e==NULL) return NULL;
    else return hash_entry(e,struct pte, elem);
}
void pt_destroy(struct hash *pt)
{
    hash_destroy(pt, pt_destroy_func);
}
void pt_destroy_func(struct hash_elem *e, void *aux)
{
    struct pte *page;
    page = hash_entry(e,struct pte, elem);
    if(page->frame!=NULL)  frame_deallocate();
    free(page);
}

/* P3-5 */
void pt_destory_by_addr (void* addr)
{
    struct pte* page = pte_find(addr);
    pte_delete(thread_current()->page_table, page);
    if(page->frame!=NULL)  frame_deallocate();
    free(page);
}

bool pte_create_by_file(void* addr, struct file* file, 
    off_t offset, size_t read_bytes, size_t zero_bytes, 
    bool writable)
{
    if(pte_find(addr) != NULL)
        return false;

    struct page* page_entry = malloc(sizeof(struct page));
    if(page_entry != NULL)
    {
        page_entry->vaddr = addr;
        page_entry->file = file;

        page_entry->offset = offset;
        page_entry->read_bytes = read_bytes;
        page_entry->zero_bytes = zero_bytes;

        page_entry->writable = writable;

        page_entry->frame = NULL;

        hash_insert(thread_current()->page_table, &page_entry->elem);
        return true;
    }
    else
    {
        return false;
    }
}