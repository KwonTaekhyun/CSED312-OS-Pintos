#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

//2-2
void argu_stack(char **argv, int argc, void **esp);

//p3
bool handle_mm_fault(struct pte *p);
#endif /* userprog/process.h */
