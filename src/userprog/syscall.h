#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <list.h>
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/fsutil.h"
#include "filesys/inode.h"
#include "filesys/off_t.h"

#define STACK_HEURISTIC 32
#include "vm/page.h"
#include "vm/frame.h"
#include "lib/user/syscall.h"

struct fd_elem {
  int fd;
  struct file* file_ptr;
  struct list_elem elem;
};

void kill_process(void);

struct lock file_lock;

void syscall_init (void);

void sys_halt (struct intr_frame * f);
void sys_exit (struct intr_frame * f);
void sys_exec (struct intr_frame * f);
void sys_wait (struct intr_frame * f);
void sys_create (struct intr_frame * f);
void sys_remove (struct intr_frame * f);
void sys_open (struct intr_frame * f);
void sys_filesize (struct intr_frame * f);
void sys_read (struct intr_frame * f);
void sys_write (struct intr_frame * f);
void sys_seek (struct intr_frame * f);
void sys_tell (struct intr_frame * f);
void sys_close (struct intr_frame * f);

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

void is_valid_address(void *esp, int start, int end);
struct file_descriptor* find_fd_by_idx(int fd_idx);
void check_buffer(void *buf, unsigned size, void *esp, bool to_write);
void check_valid_string(const void *str, void *esp);
void check_address(void *addr, void *esp);

#endif /* userprog/syscall.h */
