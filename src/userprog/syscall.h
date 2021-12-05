#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include "lib/user/syscall.h"
#define STACK_HEURISTIC 32
/* P3-5. File memory mapping */
#include "filesys/off_t.h"
#include <list.h>
#include "vm/page.h"
#include "vm/frame.h"

void syscall_init (void);

/* P2-3 */
void sys_exit(int exit_status);
pid_t sys_exec (const char *cmd);
int sys_wait (pid_t pid);

bool sys_create(const char *file , unsigned initial_size);
bool sys_remove (const char *file);

struct lock filesys_lock;

int sys_open(char *file_name);
int sys_filesize(int fd);
int sys_read (int fd, void* buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);
void sys_seek (int fd_idx, unsigned pos);
unsigned sys_tell (int fd_idx);
void sys_close(int fd_idx);

/* P3-5. File memory mapping */
struct file_mapping{
    mapid_t mapid;
    struct file *file_ptr;
    void *addr;
    struct list_elem file_mapping_elem;
    struct list pte_list;
    int file_page_num;
};

mapid_t sys_mmap(int fd_idx, void *addr);
void sys_munmap(int mapid);
void mmap_file_write_at(struct file* file, void* addr, size_t read_bytes, off_t offset);

void is_valid_address(void *esp, int start, int end);
struct file_descriptor* find_fd_by_idx(int fd_idx);
struct pte *check_addr(void *addr);
void check_buffer(void *buf, unsigned size, void *esp, bool to_write);
void check_valid_string(const void *str, void *esp);
void check_address(void *addr, void *esp);
#endif /* userprog/syscall.h */
