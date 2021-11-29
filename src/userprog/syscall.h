#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


#include "lib/user/syscall.h"
//p3
#include "vm/page.h"
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

void is_valid_address(void *esp, int start, int end);
struct file_descriptor* find_fd_by_idx(int fd_idx);
struct pte *check_addr(void *addr);
void check_buffer(void *buf, unsigned size, void *esp, bool to_write);
void check_valid_string(const void *str, void *esp);
#endif /* userprog/syscall.h */
