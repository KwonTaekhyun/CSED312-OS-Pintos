#include "vm/page.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "lib/string.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
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
    free(page);
} 
bool load_file(void *addr, struct pte *p)
{
    // P3-5-test
    // printf("load file, address: %p\n", addr);

    if(file_read_at(p->file, addr, p->read_bytes, p->offset)!=(p->read_bytes)){
        // P3-5-test
        // printf("load file fail\n");
        return false;
    }
    memset(addr+(p->read_bytes), 0, p->zero_bytes);
    return true;
}
/* bool pte_create_file(void* upage, struct file* file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, bool is_mmap)
{
    if(pte_find(upage) != NULL) return false;
    struct pte* page = malloc(sizeof(struct pte));
    if(page != NULL)
    {
        page->type = VM_BIN;
        page->file = file;
        page->writable = writable;
        page->vaddr = upage;
        page->is_loaded = false;
        page->offset = ofs;
        page->read_bytes = read_bytes;
        page->zero_bytes = zero_bytes;
        //page->frame = NULL;
        pte_insert(thread_current()->page_table, &page->elem);
        return true;
    }
    else return false;
}

bool pte_create_zero(void *upage)
{
    if(pte_find(upage) != NULL) return false;
    struct pte* page = malloc(sizeof(struct pte));    
    if(page != NULL)
    {
        page->type = VM_ANON;
        page->file = NULL;
        page->writable = true;
        page->vaddr = upage;
        page->is_loaded = false;
        page->offset = 0;
        page->read_bytes = 0;
        page->zero_bytes = 0;
        //page->frame = NULL;
        pte_insert(thread_current()->page_table, &page->elem);
        return true;
    }
    else return false;
}
bool
page_load(void *upage)
{
    struct pte* page = pte_find(upage);
    if (page == NULL) return false;
    struct frame* f = frame_allocate(page);
    if(f == NULL) return false;
    bool success;
    switch (page->type)
    {
    case VM_BIN:
        success = load_file(f, page);
        break;
    case VM_FILE:
        break;
    
    case VM_ANON:
        success = memset(f->addr, 0, PGSIZE) != NULL;
        break;

    default:
        NOT_REACHED();
        break;
    }
    
    if(!success || !pagedir_set_page(thread_current ()->pagedir, upage, f->addr, page->writable))
    {
        frame_deallocate(f, true);
        return false;
    }
    return true;
} */

/* P3-5. File memory mapping */
void pt_destory_by_addr (void* addr)
{
    struct pte* page = pte_find(addr);
    pte_delete(&(thread_current()->page_table), page);
    if(page->frame!=NULL)  frame_deallocate(page->frame->addr);
    free(page);
}

bool pte_create_by_file(void* addr, struct file* file, off_t offset, size_t read_bytes, size_t zero_bytes, bool writable, bool file_mapping)
{
    if(pte_find(addr) != NULL){
        return false;
    }

    // P3-5. File memory mapping
    // printf("address: %p, offset: %d\n", addr, offset / 4096);

    struct pte* page_entry = malloc(sizeof(struct pte));
    if(page_entry != NULL)
    {
        page_entry->vaddr = addr;
        page_entry->file = file;

        page_entry->type = file_mapping ? VM_FILE : VM_BIN;

        page_entry->offset = offset;
        page_entry->read_bytes = read_bytes;
        page_entry->zero_bytes = zero_bytes;

        page_entry->writable = writable;

        page_entry->frame = NULL;

        hash_insert(&(thread_current()->page_table), &page_entry->elem);
        return true;
    }
    else
    {
        return false;
    }
}