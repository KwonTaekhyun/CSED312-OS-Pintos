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
  is_valid_address(f->esp, 0, 3);
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
      is_valid_address(f->esp, 4, 7);
      f->eax = exec((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_WAIT:{
      is_valid_address(f->esp, 4, 7);
      f->eax = wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_CREATE:{
      char *file;
      unsigned int initial_size;
      //is_valid_address(f->esp, 4, sizeof(char*));
      //is_valid_address(f->esp, 8, sizeof(unsigned int));
      read_argument(f->esp+4,&file,sizeof(char*));
      read_argument(f->esp+8,&file,sizeof(unsigned int));
      f->eax = sys_create(file, initial_size);
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
      is_valid_address(f->esp, 4, 15);
      read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    }
    case SYS_WRITE:{
      is_valid_address(f->esp, 4, 15);
      f->eax = write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
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

bool sys_create(const char *file , unsigned initial_size)
{
  if(file == NULL) exit(-1);
  return filesys_create (file, initial_size);
}
bool sys_remove (const char *file) 
{
  return filesys_remove(file);
}
