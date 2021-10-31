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

  //1-1. register들 읽기
  void *SP = (uint32_t *)(f->esp);
  uint32_t *RT = &(f->eax);

  //1-2. stack pointer 유효 여부 검사하기
  if(!(is_user_vaddr(SP) && is_user_vaddr(SP + sizeof(uint32_t) - 1))){
    thread_exit(-1);
  }

  //2. syscall number에 따라 분기나누기
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
      int exit_code;
      if(read_argument(SP + 4, &exit_code, sizeof(int)) == -1){
        thread_exit(-1);
      }

      // 2. process 종료 ***
      thread_exit(*args);
      break;
    }
    case SYS_EXEC: {
      //2-3. process 실행

      // 1. 실행할 프로세스 이름 읽어오기
      char *file_name;
      if(read_argument(SP + 4, file_name, sizeof(char *)) == -1){
        *RT = -1;
      }

      // 2. 프로세스를 실행시킨다
      *RT = process_execute(file_name);
      break;
    }
    case SYS_WAIT: {
      //2-4. child process가 종료되기를 기다린다

      // 1. wait할 프로세스의 pid 읽어오기
      
      break;
    }
  }
}

// esp에서 n개의 인자들을 읽어오는 함수
int read_argument (void *SP, void *arg, int bytes){
  if(!(is_user_vaddr(SP) && is_user_vaddr(SP + bytes))){
    return -1;
  }

  int i;
  for(i = 0; i < bytes; i++){
    *(arg + i) = *(SP + i);
  }
  return 0;
}