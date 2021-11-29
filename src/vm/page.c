#include "vm/page.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "lib/string.h"

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
    pte_delete(pt, &pte->elem);

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
    if(page->frame!=NULL) frame_deallocate(page->frame->addr);
    free(page);
} 
bool load_file(void *addr, struct pte *p)
{
    if(file_read_at(p->file, addr, p->read_bytes, p->offset)!=p->read_bytes)
    return false;
    memset((uint8_t*)addr+p->read_bytes, 0, p->zero_bytes);
    return true;
}