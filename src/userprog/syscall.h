#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

void syscall_init (void);

/* P2-3 */
void exit(int exit_status);
pid_t exec (const char *cmd);
int wait (pid_t pid);
int sys_read (int fd, void* buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);
int syscall_filesize(int fd);
void is_valid_address(void *esp, int start, int end);

int read_argument (void *SP, void *arg, int bytes);

//fd
struct lock file_lock;
bool sys_create(const char *file , unsigned initial_size);
bool sys_remove (const char *file);
int sys_open(char *file_name);
void sys_close(int fd_idx);
struct file_descriptor* find_fd_by_idx(int fd_idx);


#endif /* userprog/syscall.h */
