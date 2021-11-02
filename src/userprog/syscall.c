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

static void
syscall_handler (struct intr_frame *f) 
{
  /* P2-3 */

  //1-1. interrupt frame으로부터 stack pointer, 
  //return value를 저장하는 레지스터 읽어오기
  void *SP = (uint32_t *)(f->esp);
  uint32_t *RT = &(f->eax);

  //1-2. stack pointer 유효 여부 검사하기
  if(!(is_user_vaddr(SP) && is_user_vaddr(SP + sizeof(uint32_t) - 1))){
    exit(-1);
  }

  //1-3. stack pointer로부터 첫번째 인자인 system call number 읽어오기
  int syscall_nr;
  if(!read_argument(SP, &syscall_nr, sizeof(int))){
    exit(-1);
  }

  //2. systam call number에 따라 분기나누기
  switch(syscall_nr){
    case SYS_HALT: {
      //2-1. system halting
      // 1. OS 자체 종료
      shutdown_power_off();
      break;
    }
    case SYS_EXIT: {
      //2-2. process 종료
      // 1. return state 읽어오기
      int exit_status;
      if(!read_argument(SP + 4, &exit_status, sizeof(int))){
        exit(-1);
      }
      // 2. process 종료
      exit(exit_status);
      break;
    }
    case SYS_CREATE: {
      //2-3.file 생성
      // 1. file name, initial_size 읽어오기
      char *file;
      unsigned initial_size;
      if(!read_argument(SP + 4, file, sizeof(uint32_t))){
        *RT = false;
        exit(-1);
      }
      if(!read_argument(SP + 8, &initial_size, sizeof(uint32_t))){
        *RT = false;
        exit(-1);
      }
      // 2. 파일 생성
      if(!file){
        *RT = false;
        exit(-1);
      }
      *RT = filesys_create(file, initial_size);
      break;
    }
    case SYS_REMOVE: {
      //2-4. file 삭제
      // 1. file name 읽어오기
      char *file;
      if(!read_argument(SP + 4, file, sizeof(uint32_t))){
        *RT = false;
        exit(-1);
      }
      // 2. 파일 삭제
      if(!file){
        *RT = false;
        exit(-1);
      }
      *RT = filesys_remove(file);
      break;
    }

    case SYS_EXEC: {
      //2-5. process 실행
      // 1. 실행할 프로세스 이름 읽어오기
      char *file_name;
      if(read_argument(SP + 4, file_name, sizeof(uint32_t)) == -1){
        *RT = -1;
        exit(-1);
      }
      // 2. 프로세스를 실행시킨다
      *RT = process_execute(file_name);
      break;
    }
    case SYS_WAIT: {
      //2-6. child process가 종료되기를 기다린다
      // 1. wait할 프로세스의 pid 읽어오기
      break;
    }
    case SYS_OPEN: {
      //2-7.
      break;
    }
    case SYS_FILESIZE: {
      //2-8.
      break;
    }
    case SYS_READ: {
      //2-9.
      break;
    }
    case SYS_WRITE: {
      //2-10.
      break;
    }
    case SYS_SEEK: {
      //2-11.
      break;
    }
    case SYS_TELL: {
      //2-12.
      break;
    }
    case SYS_CLOSE: {
      //2-13.
      break;
    }
  }
}

// 유효한 주소를 가리키는지 확인하는 함수

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

// system call 구현 함수들
void exit(int exit_status){
  struct thread *current_thread = thread_current();
  current_thread->exit_status = exit_status;
  printf("%s: exit(%d)\n", current_thread->name, exit_status);
  thread_exit();
}