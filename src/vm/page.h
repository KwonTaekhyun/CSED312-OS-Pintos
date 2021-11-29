#include"src/lib/kernel/hash.h"

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct pte
{
	struct file* file;   
	uint8_t type;      //0:VM_BIN, 1:VM_FILE, 2:VM_ANON
	bool writable;    
	void *vaddr;     
	bool is_loaded;     
	size_t offset;    
	size_t read_bytes;     
	size_t zero_bytes; 
    struct frame *frame;

	struct hash_elem elem;   
};

bool pt_init(struct hash *pt);
static unsigned pt_hash_func(const struct hash_elem *e, void *aux);
static bool pt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);
bool pte_insert(struct hash *pt, struct pte *pte);
bool pte_delete(struct hash *pt, struct pte *pte);
struct pte *pte_find(void *vaddr);
void pt_destroy(struct hash *pt);
bool load_file(void *addr, struct pte *p);