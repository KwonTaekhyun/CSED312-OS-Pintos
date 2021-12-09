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
#include "vm/frame.h"

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
  check_address(f->esp,f->esp);
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
      //p3
      check_valid_string((const void *)*(uint32_t *)(f->esp + 4), f->esp);
      f->eax = sys_exec((const char *)*(uint32_t *)(f->esp + 4));
      pte_find((f->esp + 4))->pinned = false;
      break;
    }
    case SYS_WAIT:{
      is_valid_address(f->esp, 4, 7);
      f->eax = sys_wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_CREATE:{
      is_valid_address(f->esp, 4, 11);
      //p3
      check_valid_string((const void *)*(uint32_t *)(f->esp + 4), f->esp);
      f->eax = sys_create((const char *)*(uint32_t *)(f->esp + 4), (int)*(uint32_t *)(f->esp + 8));
      pte_find((f->esp + 4))->pinned = false;
      break;
    }
    case SYS_REMOVE:{
      is_valid_address(f->esp, 4, 7);
      check_valid_string((const void *)*(uint32_t *)(f->esp + 4), f->esp);
      f->eax = sys_remove((const char *)*(uint32_t *)(f->esp + 4));
      break;
    }
    case SYS_OPEN:{
      is_valid_address(f->esp, 4, 7);
      //p3
      check_valid_string((const void *)*(uint32_t *)(f->esp + 4), f->esp);
      f->eax = sys_open((const char *)*(uint32_t *)(f->esp + 4));
      pte_find((f->esp + 4))->pinned = false;
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
      check_buffer((void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)), f->esp, 1);
      f->eax = sys_read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      pte_find((f->esp + 8))->pinned = false;
      pte_find((f->esp + 12))->pinned = false;
      break;
    }
    case SYS_WRITE:{
      is_valid_address(f->esp, 4, 15);
      //p3
      check_buffer((void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)), f->esp, 0);
      f->eax = sys_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      pte_find((f->esp + 8))->pinned = false;
      pte_find((f->esp + 12))->pinned = false;
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
    case SYS_MMAP: {
      is_valid_address(f->esp, 4, 11);
      f->eax = sys_mmap((mapid_t)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8));
      break;
    }
    case SYS_MUNMAP: {
      is_valid_address(f->esp, 4, 7); 
      sys_munmap((mapid_t)*(uint32_t *)(f->esp + 4));
      break;
    }
  }
  pte_find(f->esp)->pinned = false;
}

// system call 구현 함수들

void sys_exit(int exit_status){
  struct thread *current_thread = thread_current();
  current_thread->exit_status = exit_status;

  printf("%s: exit(%d)\n", current_thread->name, exit_status);

  if(lock_held_by_current_thread(&filesys_lock)) lock_release(&filesys_lock);
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
  // P3-5. File memory mapping
  // printf("file size: %d", initial_size);
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
  struct file_descriptor *fd = palloc_get_page(0);

  if (!fd) {
    
    lock_release (&filesys_lock);
    return -1;
  }
  
  struct file *file_ptr = filesys_open(file_name);

  if(!file_ptr){
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
  else if(fd > 1)
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
  else if(fd > 1){
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
  lock_acquire(&filesys_lock);
  file_seek(find_fd_by_idx(fd_idx)->file_ptr, pos);
  lock_release(&filesys_lock);
}

unsigned sys_tell (int fd_idx){
  lock_acquire(&filesys_lock);
  unsigned pos = (unsigned)file_tell(find_fd_by_idx(fd_idx)->file_ptr);
  lock_release(&filesys_lock);

  return pos;
}

void sys_close(int fd_idx){
  if(fd_idx < 3){
    return;
  }

  struct file_descriptor *fd = find_fd_by_idx(fd_idx);

  list_remove(&(fd->elem));

  if(thread_current()->cur_file == fd->file_ptr){
    thread_current()->cur_file = NULL;
  }

  if(fd->file_ptr) {
    lock_acquire(&filesys_lock);
    file_close(fd->file_ptr);
    palloc_free_page(fd);
    lock_release(&filesys_lock);
  }
}

/* P3-5. File memory mapping */
mapid_t sys_mmap(int fd_idx, void *addr){

  if(fd_idx < 2 || !addr || pg_ofs(addr) != 0 || !is_user_vaddr(addr)){
    return -1;
  }

  lock_acquire (&filesys_lock);

  struct file_descriptor* fd = find_fd_by_idx(fd_idx);

  struct file* file_ptr = file_reopen(fd->file_ptr);
  if(!file_ptr){
    lock_release (&filesys_lock);
    return -1;
  }

  /* file ptr를 addr에 lazy loading */
  int i;
  off_t offset = 0;
  int file_bytes = file_length(file_ptr);
  if(!file_bytes){
    lock_release (&filesys_lock);
    return -1;
  }
  int file_page_num = file_bytes / PGSIZE;

  for(i = 0; i < file_page_num; i++){
    // ** 페이지 단위로 pte 생성 (pte_create_with_file) **
    size_t read_bytes = file_bytes - (i * PGSIZE) > PGSIZE ? PGSIZE : file_bytes - (i * PGSIZE);

    bool pte_created = pte_create_by_file(addr + offset, file_ptr, offset, 
      read_bytes, PGSIZE - read_bytes, true);

    if(!pte_created)
    {
      file_close(file_ptr);
      lock_release (&filesys_lock);
      return -1;
    }

    offset += read_bytes;
  }

  if(file_bytes - (file_page_num * PGSIZE) != 0){
    size_t read_bytes = file_bytes - offset;

    bool pte_created = pte_create_by_file(addr + offset, file_ptr, offset, 
      read_bytes, PGSIZE - read_bytes, true);

    if(!pte_created)
    {
      file_close(file_ptr);
      lock_release (&filesys_lock);
      return -1;
    }

    file_page_num++;
  }

  struct thread *current_thread = thread_current();
  struct file_mapping* file_mapping_entry = malloc(sizeof(struct file_mapping));
  file_mapping_entry->file_ptr = file_ptr;
  file_mapping_entry->addr = addr;
  file_mapping_entry->mapid = current_thread->file_mapping_num++;
  file_mapping_entry->file_page_num = file_page_num;
  list_push_back(&(current_thread->file_mapping_table), &(file_mapping_entry->file_mapping_elem));

  lock_release (&filesys_lock);

  return file_mapping_entry->mapid;
}

void sys_munmap(int mapid){
  struct thread *current_thread = thread_current();
  struct list_elem *e;
  struct file_mapping *file_mapping_entry = NULL;
  for (e = list_begin (&current_thread->file_mapping_table); 
        e != list_end (&current_thread->file_mapping_table); e = list_next (e)){
    struct file_mapping * file_mapping = list_entry (e, struct file_mapping, file_mapping_elem);
    if (file_mapping->mapid == mapid){
      file_mapping_entry = file_mapping;
      break;
    }
  }
  if(!file_mapping_entry){
    return;
  }

  lock_acquire (&filesys_lock);

  int i;
  int file_page_num = file_mapping_entry->file_page_num;
  for(i = 0; i < file_page_num; i++){
    // ** 페이지 단위로 pte 할당해제 **
    // ** 만약 frame이 할당되어 있다면 frame 제거하고 dirty할 경우 disk에 작성한다. **
    
    struct pte* page = pte_find(file_mapping_entry->addr + (i * PGSIZE));
    if(!page){
      continue;
    }
    // P3-5. File memory mapping
    if(page->frame)
    {
      if(pagedir_is_dirty (current_thread->pagedir, page->vaddr)){
        file_write_at(page->file, page->frame->addr, PGSIZE, i * PGSIZE);
      }
      frame_deallocate(page->frame->addr);
    }
    pagedir_clear_page (current_thread->pagedir, page->vaddr);
    pte_delete (&(current_thread->page_table), page);
  }
  
  list_remove(&(file_mapping_entry->file_mapping_elem));
  file_close(file_mapping_entry->file_ptr);
  free(file_mapping_entry);

  lock_release (&filesys_lock);
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

void check_buffer(void *buf, unsigned size, void *esp, bool to_write)
{
  unsigned *i;
  struct pte *page;
  for(i = (void*)buf; i<(void*)buf+size; i++)
  {
    check_address((void*)i,esp);
    page = pte_find((void*)i);
    if((page!=NULL) && (!page->writable && to_write)) sys_exit(-1); 
  }
}
void check_valid_string(const void *str, void *esp)
{
  uint8_t *i;
  struct pte *page;
  char *str_temp = (char*) str;
  while(*str_temp < 0)
	{
    check_address(str_temp, esp);
		str_temp += 1;
	}
}
void check_address(void *addr, void *esp)
{
	struct pte *page;
  if(!is_user_vaddr(addr)) sys_exit(-1);
	if(addr >= (void *)0x08048000 || addr < (void *)0xc0000000)
	{
		page = pte_find(addr);
		if(page == NULL)
		{
			if(addr >= esp-STACK_HEURISTIC){
				if(!expand_stack(addr))
					sys_exit(-1);
			}
			else
				sys_exit(-1);
		}
	}
	else sys_exit(-1);

}
