#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/signal.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

static int get_user(const uint8_t *uaddr);
static bool put_user(uint8_t *udst, uint8_t byte);
struct file* get_file_from_fd(int fd);
bool validate_read(void *p, int size);
bool validate_write(void *p, int size);

static void (*syscall_table[20])(struct intr_frame*) = {
  sys_halt,
  sys_exit,
  sys_exec,
  sys_wait,
  sys_create,
  sys_remove,
  sys_open,
  sys_filesize,
  sys_read,
  sys_write,
  sys_seek,
  sys_tell,
  sys_close,
  sys_mmap,
  sys_munmap
}; // syscall jmp table

/* Reads a byte at user virtual address UADDR.
  UADDR must be below PHYS_BASE.
  Returns the byte value if successful, -1 if a segfault
  occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
        : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
  UDST must be below PHYS_BASE.
  Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
        : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

struct file* get_file_from_fd(int fd) {

  struct list_elem *e;
  struct thread *t = thread_current();
  struct fd_elem *fd_elem;

  for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
       e = list_next (e))
  {
    fd_elem = list_entry (e, struct fd_elem, elem);
    if(fd_elem->fd == fd)
      return fd_elem->file_ptr;
  }
  return NULL;
}

bool validate_read(void *p, int size) {
  int i = 0;
  if(p >= PHYS_BASE || p + size >= PHYS_BASE) return false;
  for(i = 0; i < size; i++) {
    if(get_user(p + i) == -1)
      return false;
  }
  return true;
}

bool validate_write(void *p, int size) {
  int i = 0;
  if(p >= PHYS_BASE || p + size >= PHYS_BASE) return false;
  for(i = 0; i < size; i++) {
    if(put_user(p + i, 0) == false)
      return false;
  }
  return true;
}

void kill_process() {
  send_signal(-1, SIG_WAIT);
  printf ("%s: exit(%d)\n", thread_current()->name, -1);
  thread_exit();
}

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{ 
  int syscall_num = validate_read(f->esp, 4) ? *(int*)(f->esp) : -1;
  
  if(syscall_num < 0 || syscall_num >= 20) {
    kill_process();
  }
  
  (syscall_table[syscall_num])(f);
}

// void halt(void)
void sys_halt (struct intr_frame * f UNUSED) {
  shutdown_power_off();
}

// void exit(int status)
void sys_exit (struct intr_frame * f) {
  int status;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  status = *(int*)(f->esp + 4);

  send_signal(status, SIG_WAIT);
  printf ("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();  
}

// pid_t exec(const char *cmd_line)
void sys_exec (struct intr_frame * f) {
  char *cmd_line;
  pid_t pid;
  char *itr;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  cmd_line = *(char**)(f->esp + 4);
  itr = cmd_line;
  
  if(!validate_read((void*)cmd_line, 1)) kill_process();
  
  while(*itr != '\0') {
    itr++;
    if(!validate_read((void*)itr, 1)) kill_process();
  }
  
  pid = process_execute(cmd_line);
  f->eax = pid == -1 ? pid : get_signal(pid, SIG_EXEC);
}

// int wait (pid_t pid)
void sys_wait (struct intr_frame * f) {
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  pid_t pid = *(pid_t*)(f->esp + 4);
  
  f->eax = process_wait(pid);
}

//bool create (const char *file, unsigned initial_size)
void sys_create (struct intr_frame * f) {
  char *name;
  unsigned initial_size;
  char *itr;
  
  if(!validate_read(f->esp + 4, 8)) kill_process();
  
  name = *(char **)(f->esp + 4);
  initial_size = *(unsigned*)(f->esp + 8);
  itr = name;
  
  if(!validate_read((void*)name, 1)) kill_process();

  while(*itr != '\0') {
    itr++;
    if(!validate_read((void*)itr, 1)) kill_process();
  }

  lock_acquire(&file_lock);  
  f->eax = filesys_create(name, initial_size);
  lock_release(&file_lock);
}

//bool remove (const char *file)
void sys_remove (struct intr_frame * f) {
  char *name;
  char *itr;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  name = *(char **)(f->esp + 4);
  itr = name;
  
  if(!validate_read((void*)name, 1)) kill_process();

  while(*itr != '\0') {
    itr++;
    if(!validate_read((void*)itr, 1)) kill_process();
  }
  
  lock_acquire(&file_lock);
  f->eax = filesys_remove(name);
  lock_release(&file_lock);
}

//int open (const char *file)
void sys_open (struct intr_frame * f) {
  char *name;
  char *itr;
  struct thread *t;
  struct file *file;
  struct list_elem *e;
  struct fd_elem *f_elem;
  struct fd_elem *fd_elem;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();

  name = *(char **)(f->esp + 4);
  itr = name;

  if(!validate_read((void*)name, 1)) kill_process();

  while(*itr != '\0') {
    itr++;
    if(!validate_read((void*)itr, 1)) kill_process();
  }
  
  if(itr == name) {
    f->eax = -1;
    return;
  }
  
  t = thread_current();
  file = filesys_open(name); //if fails, it returns NULL
  f_elem = malloc(sizeof(struct fd_elem));
  
  if(file == NULL) {
    f->eax = -1;
    return;
  }

  f_elem->fd = 2;
  f_elem->file_ptr = file;

  for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
       e = list_next (e))
  {
    fd_elem = list_entry (e, struct fd_elem, elem);
    if(fd_elem->fd > f_elem->fd) {
      e = list_prev(e);
      list_insert(e, &f_elem->elem);
      f->eax = f_elem->fd;
      return;
    }
    f_elem->fd++;
  }
  list_push_back(&t->fd_table, &f_elem->elem);
  f->eax = f_elem->fd;
}

//int filesize (int fd)
void sys_filesize (struct intr_frame * f) {
  int fd;
  struct file *file;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  fd = *(int*)(f->esp + 4);
  file = get_file_from_fd(fd);
  
  if(file == NULL) f->eax = -1;
  
  f->eax = file_length(file);
}

//int read (int fd, void *buffer, unsigned size)
void sys_read (struct intr_frame * f) {
  char c;
  unsigned count = 0;
  int fd;
  uint8_t* buffer;
  unsigned size;
  struct file *file;
  
  if(!validate_read(f->esp + 4, 12)) kill_process();
  
  fd = *(int*)(f->esp + 4);
  buffer = *(uint8_t**)(f->esp + 8);
  size = *(unsigned*)(f->esp + 12);
  file = get_file_from_fd(fd); 
  
  if(!validate_write(buffer, size)) kill_process();
  
  if(fd == 0) {
    c = input_getc();
    while(c != '\n' && c != -1 && count < size) {
      if(!put_user(buffer, c)) kill_process();
      buffer++;
      count++;
      c = input_getc();
    }
    f->eax = count;
  }
  else if(fd == 1) {
    f->eax = -1;
  }
  else {
    if(file == NULL) {
      f->eax = -1;
      return;
    }
    lock_acquire(&file_lock);
    f->eax = file_read(file, buffer, size);
    lock_release(&file_lock);
  }
}

//int write (int fd, const void *buffer, unsigned size)
void sys_write (struct intr_frame * f) {
  int fd;
  char* buffer;
  unsigned size;
  struct file *file;
  
  if(!validate_read(f->esp + 4, 12)) kill_process();
  
  fd = *(int*)(f->esp + 4);
  buffer = *(char**)(f->esp + 8);
  size = *(unsigned*)(f->esp + 12);
  file = get_file_from_fd(fd);
  
  if(!validate_read(buffer, size)) kill_process();
  
  if(fd == 0) {
    f->eax = 0; 
  }
  else if(fd == 1) {
    putbuf(buffer, size);
    f->eax = size;
  }
  else {
    if(file == NULL) {
      f->eax = 0;
      return;
    }
    lock_acquire(&file_lock);
    f->eax = file_write (file, buffer, size);
    lock_release(&file_lock);
  }
}

//void seek (int fd, unsigned position)
void sys_seek (struct intr_frame * f) {
  int fd;
  off_t position;
  struct file *file;
  
  if(!validate_read(f->esp + 4, 8)) kill_process();
  
  fd = *(int*)(f->esp + 4);
  position = *(int*)(f->esp + 8);
  file = get_file_from_fd(fd);  
  
  if(file == NULL) f->eax = -1;
  
  lock_acquire(&file_lock);
  file_seek(file, position);
  lock_release(&file_lock);
}

//unsigned tell (int fd)
void sys_tell (struct intr_frame * f) {
  if(!validate_read(f->esp + 4, 4)) kill_process();
  int fd = *(int*)(f->esp + 4);
  struct file *file = get_file_from_fd(fd);
  if(file == NULL)
    f->eax = -1;
  lock_acquire(&file_lock);
  f->eax = file_tell(file);
  lock_release(&file_lock);
}

//void close (int fd)
void sys_close (struct intr_frame * f) {
  int fd;
  struct file *file;
  struct thread *t;
  struct list_elem *e;
  struct fd_elem *fd_elem;
  
  if(!validate_read(f->esp + 4, 4)) kill_process();
  
  fd = *(int*)(f->esp + 4);
  file = get_file_from_fd(fd);
  t = thread_current();
    
  lock_acquire(&file_lock);
  file_close(file);
  lock_release(&file_lock);
  
  for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
       e = list_next (e))
  {
    fd_elem = list_entry (e, struct fd_elem, elem);
    if(fd_elem->fd == fd) {
      list_remove(e);
      free(fd_elem);
      return;
    }
  }
}

/* P3-5. File memory mapping */
mapid_t sys_mmap(int fd_idx, void *addr){

  if(fd_idx < 2 || !addr || pg_ofs(addr) != 0 || !is_user_vaddr(addr)){
    return -1;
  }

  lock_acquire (&file_lock);

  struct thread *t;
  struct list_elem *e;
  struct fd_elem *fd_elem;
  t = thread_current();

  for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
       e = list_next (e))
  {
    fd_elem = list_entry (e, struct fd_elem, elem);
    if(fd_elem->fd == fd_idx) {
      break;
    }
  }

  struct file* file_ptr = file_reopen(fd_elem->file_ptr);
  if(!file_ptr){
    lock_release (&file_lock);
    return -1;
  }

  /* file ptr를 addr에 lazy loading */
  int i;
  off_t offset = 0;
  int file_bytes = file_length(file_ptr);
  if(!file_bytes){
    lock_release (&file_lock);
    return -1;
  }
  int file_page_num = file_bytes / PGSIZE;

    // P3-5-test
    // printf("file_bytes: %d, file_page_num: %d\n", file_bytes, file_page_num);

  for(i = 0; i < file_page_num; i++){
    // ** 페이지 단위로 pte 생성 (pte_create_with_file) **
    size_t read_bytes = file_bytes - (i * PGSIZE) > PGSIZE ? PGSIZE : file_bytes - (i * PGSIZE);

    bool pte_created = pte_create_by_file(addr + offset, file_ptr, offset, 
      read_bytes, PGSIZE - read_bytes, true, true);

    if(!pte_created)
    {
      file_close(file_ptr);
      lock_release (&file_lock);
      return -1;
    }

    // P3-5-test
    // printf("pte created success?: %d, %dbytes\n", pte_created, read_bytes);

    offset += read_bytes;
  }

  if(file_bytes - (file_page_num * PGSIZE) != 0){
    size_t read_bytes = file_bytes - offset;

    bool pte_created = pte_create_by_file(addr + offset, file_ptr, offset, 
      read_bytes, PGSIZE - read_bytes, true, true);

    if(!pte_created)
    {
      file_close(file_ptr);
      lock_release (&file_lock);
      return -1;
    }

    file_page_num++;

    // P3-5-test
    // printf("pte created success?: %d, %dbytes\n", pte_created, read_bytes);
  }

  // P3-5-test
  // char *buf = (char *)addr;
  // for(i = 0; i < file_bytes; i++){
  //   printf("i: %d, address %p :  %hhx\n", i, buf + i, buf[i]); 
  // }

  struct thread *current_thread = thread_current();
  struct file_mapping* file_mapping_entry = malloc(sizeof(struct file_mapping));
  file_mapping_entry->file_ptr = file_ptr;
  file_mapping_entry->addr = addr;
  file_mapping_entry->mapid = current_thread->file_mapping_num++;
  file_mapping_entry->file_page_num = file_page_num;
  list_push_back(&(current_thread->file_mapping_table), &(file_mapping_entry->file_mapping_elem));

  lock_release (&file_lock);

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

  lock_acquire (&file_lock);

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
    // printf("Is Dirty?: %s\n", pagedir_is_dirty (current_thread->pagedir, page->vaddr) ? "T" : "F");
    if(pagedir_is_dirty (current_thread->pagedir, page->vaddr)){
        // P3-5. File memory mapping
        // printf("There are something to write (dirty)\n");
        file_write_at(page->file, page->vaddr, PGSIZE, i * PGSIZE);
    }
    if(page->frame)
    {
      if(pagedir_is_dirty (current_thread->pagedir, page->vaddr)){
        // P3-5. File memory mapping
        // printf("There are something to write (dirty)\n");
        file_write_at(page->file, page->frame->addr, PGSIZE, i * PGSIZE);
      }
      frame_deallocate(page->frame->addr);
    }
    pagedir_clear_page (current_thread->pagedir, page->vaddr);
    hash_delete (&(current_thread->page_table), &(page->elem));
  }
  
  list_remove(&(file_mapping_entry->file_mapping_elem));
  file_close(file_mapping_entry->file_ptr);
  free(file_mapping_entry);

  lock_release (&file_lock);
}

void mmap_file_write_at(struct file* file, void* addr, size_t read_bytes, off_t offset)
{
  lock_acquire(&file_lock);
  file_write_at(file, addr, read_bytes, offset);
  lock_release(&file_lock);
}