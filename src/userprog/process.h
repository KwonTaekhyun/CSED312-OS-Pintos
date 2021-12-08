#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "lib/user/syscall.h"
#include "threads/thread.h"

struct process
{
    /* Owned by process.c. */
    const char *file_name; /* File name to execute. */

    /* Shared between process.c and syscall.c. */
    pid_t pid;                  /* Process identifier. */
    struct thread *parent;      /* Parent process. */
    struct list_elem childelem; /* List element for children list. */
    bool is_loaded;             /* Whether program is loaded. */
    struct semaphore sema_load; /* Semaphore for waiting until load. */
    bool is_exited;             /* Whether process is exited. */
    struct semaphore sema_exit; /* Semaphore for waiting until exit. */
    int exit_status;            /* Exit status. */
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

//2-2
void argu_stack(char **argv, int argc, void **esp);

//p3
bool handle_mm_fault(struct pte *p);
#endif /* userprog/process.h */
