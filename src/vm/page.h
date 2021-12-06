
#ifndef PAGE_H
#define PAGE_H

#include <hash.h>

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

/* P3-5. File memory mapping */
#include "filesys/off_t.h"

struct pte
{
	struct file* file;   
	uint8_t type;      //0:VM_BIN, 1:VM_FILE, 2:VM_ANON
	bool writable;    
	void *vaddr;     
	bool is_loaded;
	bool pinned;
	size_t offset;    
	size_t read_bytes;     
	size_t zero_bytes; 

	struct hash_elem elem;   

	/* P3-6. Swapping */
	size_t swap_index;
	struct thread* thread;
};

bool pt_init(struct hash *pt);
static unsigned pt_hash_func(const struct hash_elem *e, void *aux);
static bool pt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);
bool pte_insert(struct hash *pt, struct pte *pte);
bool pte_delete(struct hash *pt, struct pte *pte);
struct pte *pte_find(void *vaddr);
void pt_destroy(struct hash *pt);
void pt_destroy_func(struct hash_elem *e, void *aux);
bool load_file(struct frame *frame, struct pte *p);

/* P3-5. File memory mapping */
void pt_destory_by_addr (void* addr);
bool pte_create_by_file(void* addr, struct file* file, off_t offset, size_t read_bytes, size_t zero_bytes, bool writable, bool file_mapping);

#endif
