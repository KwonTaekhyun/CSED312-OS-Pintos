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
#include "lib/kernel/list.h"
#include "filesys/file.h"

#include "lib/debug.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

/* P2-3 */

static void
syscall_handler (struct intr_frame *f) 
{
  //test
  printf("syscall-nr: %d", *(uint32_t *)(f->esp));

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
      is_valid_address(f->esp, 4, 11);
      f->eax = sys_create((const char *)*(uint32_t *)(f->esp + 4), (int)*(uint32_t *)(f->esp + 8));
      break;
    }
    case SYS_REMOVE:{
      break;
    }
    case SYS_OPEN:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_open((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_FILESIZE:{
      int fd;
      is_valid_address(f->esp, 4, 7); 
      f->eax = syscall_filesize((int)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_READ:{
      is_valid_address(f->esp, 4, 7);
      is_valid_address(f->esp, 8, 11);
      is_valid_address(f->esp, 12, 15);
      f->eax = sys_read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    }
    case SYS_WRITE:{
      is_valid_address(f->esp, 4, 15);
      f->eax = sys_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    }
    case SYS_SEEK:{
      break;
    }
    case SYS_TELL:{
      break;
    }
    case SYS_CLOSE:{
      is_valid_address(f->esp, 4, 7);
      sys_close((int)*(uint32_t *)(f->esp + 4));
      break;
    }
  }
}

// system call 구현 함수들

void exit(int exit_status){
  struct thread *current_thread = thread_current();
  current_thread->exit_status = exit_status;

  //test
  debug_backtrace();

  printf("%s: exit(%d)\n", current_thread->name, exit_status);
  thread_exit();
}

pid_t exec (const char *cmd) {
  return process_execute(cmd);
}

int wait (pid_t pid) {
  return process_wait(pid);
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

int sys_open(char *file_name){
  if(!file_name){
    return -1;
  }
  lock_acquire(&file_lock);
  struct file *file_ptr = filesys_open(file_name);

  if(!file_ptr || !is_user_vaddr(file_ptr)){
    return -1;
  }

  struct file_descriptor *fd;
  fd = palloc_get_page(0);
  fd->file_pt = file_ptr;

  struct list *fd_list_ptr = &(thread_current()->file_descriptor_list);
  if(list_empty(fd_list_ptr)){
    fd->index = 3;
  }
  else{
    fd->index = (list_entry(list_back(fd_list_ptr), struct file_descriptor, elem)->index) + 1;
  }
  list_push_back(fd_list_ptr, &fd->elem);

  lock_release(&file_lock);

  return fd->index;
}

int syscall_filesize(int fd)
{
  struct file *f;
  f = find_fd_by_idx(fd)->file_pt;
  if(f == NULL) return -1;
  else
  {
    lock_acquire(&file_lock);
    off_t temp = file_length(f);
    lock_release(&file_lock);

    return temp;
  }
}

int sys_read (int fd, void* buffer, unsigned size) {
  if (fd == 0)
  {
    int i;
    uint8_t *temp_buf = (uint8_t *) buffer;
    for(i = 0; i < size; i++)
    {
      temp_buf[i] = input_getc();
    }
    return size;
  }
  else if(fd > 2)
  {
    struct file *f = find_fd_by_idx(fd)->file_pt;
    if(f==NULL) 
    {
      exit(-1);
    }

    lock_acquire(&file_lock);
    off_t temp = file_read(f,buffer,size);
    lock_release(&file_lock);

    return temp;
  }
}

int sys_write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

void sys_close(int fd_idx){
  if(fd_idx < 3){
    return;
  }

  struct file_descriptor *fd = find_fd_by_idx(fd_idx);

  list_remove(&(fd->elem));

  if(fd->file_pt) {
    file_close(fd->file_pt);
    palloc_free_page(fd);
  }
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

struct file_descriptor* find_fd_by_idx(int fd_idx){
  struct file_descriptor *fd;
  struct list_elem *fd_elem = list_begin(&thread_current()->file_descriptor_list);

  while(fd_elem != list_end(&thread_current()->file_descriptor_list)){
    fd = list_entry(fd_elem, struct file_descriptor, elem);
    if(fd_idx == fd->index){
       break;
    }
    fd_elem = list_next(fd_elem);
    if(fd_elem == list_end(&thread_current()->file_descriptor_list)){
      exit(-1);
    }
  }
  return fd;
}