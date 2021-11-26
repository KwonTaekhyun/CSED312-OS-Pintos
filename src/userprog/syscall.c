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
#include "lib/user/syscall.h"
#include "threads/palloc.h"
//p3
#include "vm/page.h"

struct file 
{
  struct inode *inode;        /* File's inode. */
  off_t pos;                  /* Current position. */
  bool deny_write;            /* Has file_deny_write() been called? */
};

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init (&filesys_lock);
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
      sys_exit(*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_EXEC:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_exec((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_WAIT:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_CREATE:{
      is_valid_address(f->esp, 4, 11);
      f->eax = sys_create((const char *)*(uint32_t *)(f->esp + 4), (int)*(uint32_t *)(f->esp + 8));
      break;
    }
    case SYS_REMOVE:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_remove((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_OPEN:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_open((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_FILESIZE:{
      is_valid_address(f->esp, 4, 7); 
      f->eax = sys_filesize((int)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_READ:{
      is_valid_address(f->esp, 4, 15);
      //p3
      //check_buffer(f->esp + 8, 4, esp,)
      f->eax = sys_read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    }
    case SYS_WRITE:{
      is_valid_address(f->esp, 4, 15);
      f->eax = sys_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    }
    case SYS_SEEK:{
      is_valid_address(f->esp, 4, 11);
      sys_seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    }
    case SYS_TELL:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_tell((int)*(uint32_t *)(f->esp + 4));
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

void sys_exit(int exit_status){
  struct thread *current_thread = thread_current();
  current_thread->exit_status = exit_status;

  file_close(current_thread->cur_file);

  struct list *fd_list = &current_thread->file_descriptor_list;
  while (!list_empty(fd_list)) {
    struct list_elem *e = list_pop_front (fd_list);
    struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
    
    file_close(fd->file_ptr);
    palloc_free_page(fd);
  }

  printf("%s: exit(%d)\n", current_thread->name, exit_status);
  thread_exit();
}

pid_t sys_exec (const char *cmd) {
  return process_execute(cmd);
}

int sys_wait (pid_t pid) {
  return process_wait(pid);
}

bool sys_create(const char *file , unsigned initial_size)
{
  if(file == NULL){
    sys_exit(-1);
  }
  return filesys_create (file, initial_size);
}

bool sys_remove (const char *file) 
{
  return  filesys_remove(file);
}

int sys_open(char *file_name){
  if(!file_name){
    return -1;
  }

  lock_acquire (&filesys_lock);

  struct file *file_ptr = filesys_open(file_name);

  if(!file_ptr){
    lock_release (&filesys_lock);
    return -1;
  }

  struct file_descriptor *fd = palloc_get_page(0);

  if (!fd) {
    palloc_free_page (fd);
    lock_release (&filesys_lock);
    return -1;
  }

  fd->file_ptr = file_ptr;

  struct list *fd_list_ptr = &(thread_current()->file_descriptor_list);
  if(list_empty(fd_list_ptr)){
    fd->index = 3;
  }
  else{
    fd->index = (list_entry(list_back(fd_list_ptr), struct file_descriptor, elem)->index) + 1;
  }

  list_push_back(fd_list_ptr, &fd->elem);
  lock_release (&filesys_lock);
  return fd->index;
}

int sys_filesize(int fd)
{
  struct file *f;
  f = find_fd_by_idx(fd)->file_ptr;

  if(f == NULL){
    return -1;
  }
  else
  {
    return (int) file_length(f);
  }
}

int sys_read (int fd, void* buffer, unsigned size) {
  lock_acquire (&filesys_lock);

  if (fd == 0){
    uint8_t *temp_buf = (uint8_t *) buffer;
    int i;
    for(i = 0; i < size; i++){
      temp_buf[i] = input_getc();
    }
    lock_release (&filesys_lock);
    return size;
  }
  else if(fd > 2)
  {
    struct file *f = find_fd_by_idx(fd)->file_ptr;
    if(f == NULL || !is_user_vaddr(buffer)){
      lock_release (&filesys_lock);
      sys_exit(-1);
    }

    int read_bytes = (int) file_read(f, buffer, size);

    lock_release (&filesys_lock);
    return read_bytes;
  }

  lock_release (&filesys_lock);
  return -1;
}

int sys_write (int fd, const void *buffer, unsigned size) {
  lock_acquire (&filesys_lock);

  if (fd == 1) {
    putbuf(buffer, size);
    lock_release (&filesys_lock);
    return size;
  }
  else if(fd > 2){
    struct file *f = find_fd_by_idx(fd)->file_ptr;
    if(f == NULL) 
    {
      lock_release (&filesys_lock);
      sys_exit(-1);
    }

    int temp = (int) file_write(f, buffer, size);

    lock_release (&filesys_lock);
    return temp;
  }

  lock_release (&filesys_lock);
  return -1; 
}

void sys_seek (int fd_idx, unsigned pos){
  file_seek(find_fd_by_idx(fd_idx)->file_ptr, pos);
}

unsigned sys_tell (int fd_idx){
  return file_tell(find_fd_by_idx(fd_idx)->file_ptr);
}

void sys_close(int fd_idx){
  if(fd_idx < 3){
    return;
  }

  struct file_descriptor *fd = find_fd_by_idx(fd_idx);

  list_remove(&(fd->elem));

  if(thread_current()->cur_file == fd->file_ptr){
    thread_current()->cur_file == NULL;
  }

  if(fd->file_ptr) {
    file_close(fd->file_ptr);
    palloc_free_page(fd);
  }
}

// 유효한 주소를 가리키는지 확인하는 함수
void is_valid_address(void *esp, int start, int end){
  if(!is_user_vaddr(esp + start) || !is_user_vaddr(esp + end) ){
    sys_exit(-1);
  }
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
      sys_exit(-1);
    }
  }
  return fd;
}
struct pte *check_addr(void *addr)
{
    return pte_find(addr);
}
/* void check_buffer(void *buf, unsigned size)
{
  uint8_t *i;
  for(i = buf; i<(uint8_t)buf+size; (uint8_t)buf+=PGSIZE)
  {
    if(check_addr(i)->writable == false) sys_exit(-1); 
  }
} */