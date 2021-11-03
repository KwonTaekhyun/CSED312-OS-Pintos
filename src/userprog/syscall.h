#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

void syscall_init (void);

/* P2-3 */
void exit(int exit_status);
pid_t exec (const char *cmd);
int wait (pid_t pid);
int read (int fd, void* buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);

void is_valid_address(void *esp, int start, int end);

int read_argument (void *SP, void *arg, int bytes);


#endif /* userprog/syscall.h */
