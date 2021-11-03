#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* P2-3 */
#include "threads/vaddr.h"
#include "lib/syscall-nr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* P2-3 */

static void
syscall_handler (struct intr_frame *f) 
{
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT:{
      shutdown_power_off();
      break;
    }
    case SYS_EXIT:{
      is_valid_address(f->esp, 4, 7);
      exit(*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_EXEC:{
      check_user_vaddr(f->esp, 4, 7);
      exec((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_WAIT:{
      check_user_vaddr(f->esp + 4);
      wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_CREATE:{
      break;
    }
    case SYS_REMOVE:{
      break;
    }
    case SYS_OPEN:{
      break;
    }
    case SYS_FILESIZE:{
      break;
    }
    case SYS_READ:{
      is_valid_address(f->esp, 20, 31);
      read((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    }
    case SYS_WRITE:{
      is_valid_address(f->esp, 20, 31);
      f->eax = write((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    }
    case SYS_SEEK:{
      break;
    }
    case SYS_TELL:{
      break;
    }
    case SYS_CLOSE:{
      break;
    }
  }
}

// system call 구현 함수들

void exit(int exit_status){
  struct thread *current_thread = thread_current();
  current_thread->exit_status = exit_status;
  printf("%s: exit(%d)\n", current_thread->name, exit_status);
  thread_exit();
}

pid_t exec (const char *cmd) {
  return process_execute(cmd);
}

int wait (pid_t pid) {
  return process_wait(pid);
}

int read (int fd, void* buffer, unsigned size) {
  int i;
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  }
  return i;
}

int write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

// 유효한 주소를 가리키는지 확인하는 함수
void is_valid_address(void *esp, int start, int end){
  if(!is_user_vaddr(esp + start) || !is_user_vaddr(esp + end) ){
    exit(-1);
  }
}

// esp에서 n개의 인자들을 읽어오는 함수
int read_argument (void *SP, void *arg, int bytes){
  if(!(is_user_vaddr(SP) && is_user_vaddr(SP + bytes))){
    return false;
  }

  int i;
  for(i = 0; i < bytes; i++){
    *(char *)(arg + i) = *(char *)(SP + i);
  }
  return true;
}

