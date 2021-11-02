#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* P2-3 */
void exit(int exit_status);
bool read_argument (void *SP, void *arg, int bytes);


#endif /* userprog/syscall.h */
