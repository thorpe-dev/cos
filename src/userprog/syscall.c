#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "process.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "devices/input.h"
#include <string.h>
#include "userprog/exception.h"

#define MAXCHAR 512

static void syscall_handler (struct intr_frame *);

static void syscall_halt    (void);
static void syscall_exit    (int status);
static void syscall_exec    (uint32_t* eax, const char* command);
static void syscall_wait    (uint32_t* eax, pid_t pid);
static void syscall_create  (uint32_t* eax, const char* file, unsigned initial_size);
static void syscall_remove  (uint32_t* eax, const char* file);
static void syscall_open    (uint32_t* eax, const char* file_name);
static void syscall_filesize(uint32_t* eax, int fd);
static void syscall_read    (uint32_t* eax, int fd, void* buffer, unsigned int size);
static void syscall_write   (uint32_t* eax, int fd, const void* buffer, unsigned size);
static void syscall_seek    (int fd, unsigned int position);
static void syscall_tell    (uint32_t* eax, int fd);
static void syscall_close   (int fd);
static void syscall_mmap    (uint32_t* eax, int fd, const void* addr);
static void syscall_munmap  (mapid_t mapid);

static void check_safe_ptr (const void* ptr, int no_args);
static void check_buffer_safety (const void* buffer, int size);
static bool check_pages (const void* addr, int size, struct sup_table* sup);


static void syscall_return_int    (uint32_t* eax, const int value);
static void syscall_return_pid_t  (uint32_t* eax, const pid_t value);
static void syscall_return_bool   (uint32_t* eax, const bool value);
static void syscall_return_uint   (uint32_t* eax, const unsigned value);
static void syscall_return_mapid_t (uint32_t* eax, const mapid_t value);


static struct file* find_file (int fd);
static struct mmap_file* find_mmap (mapid_t id);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  
  void* esp = f->esp;
  uint32_t* eax = &f->eax;
  unsigned int call_number = 0xDEADBEEF;
  
  check_safe_ptr(esp, 1);
  call_number = *(uint32_t*)esp;

  void* argument_1 = esp + 1 * sizeof(uint32_t);
  void* argument_2 = esp + 2 * sizeof(uint32_t);
  void* argument_3 = esp + 3 * sizeof(uint32_t);

  switch(call_number)
  {
    case SYS_HALT: 
      syscall_halt(); 
      break;
      
    case SYS_EXIT:
      check_safe_ptr (argument_1, 1);
      syscall_exit(*(int*)argument_1); 
      break;
      
    case SYS_EXEC:
      check_safe_ptr (argument_1, 1);
      syscall_exec(eax, *(const char**)argument_1); 
      break;
      
    case SYS_WAIT:
      check_safe_ptr (argument_1, 1);
      syscall_wait(eax, *((pid_t*)argument_1)); 
      break;
      
    case SYS_CREATE:
      check_safe_ptr (argument_1, 2);
      syscall_create(eax, *(const char**)argument_1, *(unsigned int*)argument_2); 
      break;
      
    case SYS_REMOVE: 
      check_safe_ptr (argument_1, 1);
      syscall_remove(eax, *(const char**)argument_1); 
      break;
      
    case SYS_OPEN:
      check_safe_ptr (argument_1, 1);
      syscall_open(eax, *(const char**)argument_1); 
      break;
      
    case SYS_FILESIZE:
      check_safe_ptr (argument_1, 1);
      syscall_filesize(eax, *(int*)argument_1); 
      break;
      
    case SYS_READ:
      check_safe_ptr (argument_1, 3);
      syscall_read(eax, *(int*)argument_1, *(void**)argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_WRITE: 
      check_safe_ptr (argument_1, 3);
      syscall_write(eax, *(int*)argument_1, *(void**)argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_SEEK: 
      check_safe_ptr (argument_1, 2);
      syscall_seek(*(int*)argument_1, *(unsigned int*)argument_2); 
      break;
      
    case SYS_TELL:
      check_safe_ptr (argument_1, 1);
      syscall_tell(eax, *(int*)argument_1); 
      break;
      
    case SYS_CLOSE: 
      check_safe_ptr (argument_1, 1);
      syscall_close(*(int*)argument_1); 
      break;
      
    case SYS_MMAP:
      check_safe_ptr (argument_1, 2);
      syscall_mmap(eax, *(int*)argument_1, *(void**)argument_2);
      break;
      
    case SYS_MUNMAP:
      check_safe_ptr (argument_1, 1);
      syscall_munmap((*(mapid_t*)argument_1));
      break;
      
    default: 
      printf("Invalid syscall: %d\n", call_number);
  }
}


static void
syscall_halt(void) 
{
  printf("HALT!\n");
  shutdown_power_off();
}

static void
syscall_exit(int status) 
{
  struct thread* t = thread_current();
  t->process->exit_status = status;

  thread_exit();
  NOT_REACHED ();
}

static void 
syscall_exec(uint32_t* eax, const char *command)
{
  if (!is_safe_ptr(command)) 
    thread_exit();
  syscall_return_int (eax, process_execute(command));
}

static void 
syscall_wait(uint32_t* eax, pid_t pid)
{
  syscall_return_pid_t (eax, process_wait(pid));
}

/* Lock filesystem, create file, unlock, return bool for success*/
static void 
syscall_create(uint32_t* eax, const char *filename, unsigned int initial_size)
{
  bool success = false;
  if (!is_safe_ptr(filename)) 
    thread_exit();

  lock_acquire(&filesys_lock);
  success = filesys_create(filename, initial_size);
  lock_release(&filesys_lock);
  
  syscall_return_bool (eax, success);
}

/* Lock filesystem, remove file, unlock */
static void 
syscall_remove(uint32_t* eax, const char *file)
{
  bool success = false;
  if (!is_safe_ptr(file)) 
    thread_exit();
  
  lock_acquire(&filesys_lock);
  success = filesys_remove(file);
  lock_release(&filesys_lock);
  
  syscall_return_bool (eax, success);
}


static void 
syscall_open(uint32_t* eax, const char *file_name)
{
  struct file* file;
  if (!is_safe_ptr(file_name)) 
    thread_exit();
  
  struct thread* t = thread_current();
  int fd = t->process->next_fd++;

  /* Lock filesystem, open file, unlock */
  lock_acquire(&filesys_lock);
  file = filesys_open(file_name);
  lock_release(&filesys_lock);

  /* If file not found, set eax to -1*/
  if (file == NULL) 
    syscall_return_int(eax, -1);
  else {
    /* Adds file to process' open_files list and returns fd */
      file->fd = fd;
      list_push_back(&t->process->open_files, &file->elem);
      syscall_return_int (eax, fd);
  }
}

static void 
syscall_filesize(uint32_t* eax, int fd) 
{
  int size;
  struct file* file;

  file = find_file ( fd );

  /* If fd is incorrect, return -1 */
  if (file == NULL)
    syscall_return_int(eax, -1);

  else 
  {
    lock_acquire(&filesys_lock);
    size = (int) file_length(file);
    lock_release(&filesys_lock);
    syscall_return_int (eax, size);
  }
}

static void 
syscall_read(uint32_t* eax, int fd, void* buffer, unsigned int size)
{
  
  check_buffer_safety(buffer, size);
  
  load_buffer_pages(buffer, size);
  
  struct file* file;
  int read_size = 0;
  
  /* If fd is 0, read from console */
  if (fd == 0) {

    while ((unsigned int)read_size < size) {
      *(char*)buffer = input_getc();
      read_size++;
      buffer += sizeof(uint8_t);
    }
    
  }
  /* Otherwise, read from file */
  else {
    
    file = find_file ( fd );
    
    /* If fd is incorrect, return -1 */
    if (file == NULL)
      syscall_return_int(eax, -1);
    /* Lock filesystem, read file, unlock */
    else 
    {
      lock_acquire(&filesys_lock);
      read_size = (int) file_read(file, buffer, size);
      lock_release(&filesys_lock);
    
      syscall_return_int (eax, read_size);
    }
  }
}

static void
syscall_write(uint32_t* eax, int fd, const void *buffer, unsigned int size)
{
  check_buffer_safety(buffer, size);
  
  int write_size;
  struct file* file;
  
  /* Writes to console, in blocks < maxchar */
  if (fd == 1) {
    for (write_size = size; write_size > 0; write_size -= MAXCHAR) {
      if (write_size < MAXCHAR)
        putbuf((char*)buffer, write_size);

      else {
        putbuf((char*)buffer, MAXCHAR);
        buffer += MAXCHAR;
      }
      syscall_return_int (eax, size);
    }
  }
  /* Write to file fd.*/
  else {
    file = find_file(fd);
    
    /* If fd is incorrect, return -1 */
    if (file == NULL)
      syscall_return_int(eax, -1);
    
    else {
      
      /* Lock filesystem, write to file, unlock */
      lock_acquire(&filesys_lock);
      write_size = (int)file_write(file, buffer, size);
      lock_release(&filesys_lock);
      
      syscall_return_int(eax, write_size);
    }
  }
}

static void 
syscall_seek (int fd, unsigned position) 
{
  struct file* file;
  
  file = find_file ( fd );
  
  /* If fd is incorrect, exit */
  if (file == NULL)
    return;

  else
  {
    /* Lock filesystem, seek to position in file, unlock */
    lock_acquire(&filesys_lock);
    file_seek(file, (off_t)position);
    lock_release(&filesys_lock);
  }
}

static void 
syscall_tell (uint32_t* eax, int fd) 
{
  int position;
  struct file* file;
  
  file = find_file ( fd );
  
  /* If fd is incorrect, return -1 */
  if (file == NULL)
    syscall_return_uint (eax, -1);
  
  else 
  {
    /* Lock filesystem, read file position, unlock */
    lock_acquire(&filesys_lock);
    position = (int) file_tell(file);
    lock_release(&filesys_lock);
  
    syscall_return_uint (eax, position);
  }
}

static void 
syscall_close (int fd) 
{
  struct file* file;
  
  file = find_file (fd);
  /* If fd is incorrect, exit */
  if (file == NULL)
    return;
  
  else
  {
    /* Lock filesystem, close file, unlock */
    lock_acquire(&filesys_lock);
    list_remove(&file->elem);
    if (!file->mmaped) {
      file_close(file); }
    else
      file->closed = true;
    lock_release(&filesys_lock);
  }

}

static void 
syscall_mmap  (uint32_t* eax, int fd, const void* addr)
{
  mapid_t value;
  struct file* file;
  struct mmap_file* mmap;
  off_t read_bytes;
  uint32_t zero_bytes;
  value = -1; /* Failure return value */
  
  struct sup_table* sup = thread_current()->process->sup_table;
  
  if (((int)addr % PGSIZE == 0)
    && ((int)addr != 0)
    && (fd > 1))
  {
    file = find_file (fd);
    lock_acquire (&filesys_lock);

    if (file == NULL)
      lock_release (&filesys_lock);
    
    else if (file_length (file) == 0)
      lock_release (&filesys_lock);
    
    else if (!check_pages (addr, file_length (file), sup))
      lock_release (&filesys_lock);
    
    else
    { 
      /* Map file into memory here*/
      read_bytes = file_length(file);
      zero_bytes = PGSIZE - (read_bytes % PGSIZE);
      if (load_segment (file, 0, (uint8_t*)addr, (uint32_t)read_bytes, zero_bytes, !file->deny_write))
      {
        mmap = malloc(sizeof(struct mmap_file));
        if (mmap != NULL)
        {
          value = fd;
          mmap->value = value;
          mmap->file = file;
          mmap->addr = (void*)addr;
          mmap->file_size = read_bytes;
          
          list_push_back(&thread_current()->process->mmaped_files, &mmap->elem);
        }
      }
      file->mmaped = true;
      lock_release(&filesys_lock);
    }
  }   
  
  
  syscall_return_mapid_t(eax, value);
}


static void 
syscall_munmap (mapid_t mapid) 
{
  struct mmap_file* m;

  m = find_mmap(mapid);
  
  /* If the mapid does not relate to any current mappings - return */
  if (m == NULL)
    return;
  
  un_map_file (m, true);
}

void
un_map_file (struct mmap_file* m, bool kill_thread)
{
  struct sup_table* sup;
  unsigned int i;
  struct page* p;
  struct file* file;
  
  sup = thread_current()->process->sup_table;
  
  file = m->file;

  
  /*  Go through pages for mapped file - if the page is null do nothing 
      If it hasn't been loaded - do nothing
      If its in memory, check dirty bit
      If its not in memory - it has been written to - write it back */
  for (i = 0; i <= m->file_size / PGSIZE ; i++) {
    p = page_find ((uint8_t*)m->addr + (i * PGSIZE), sup);
    
    if (p != NULL && p->loaded) {
      if (p->valid) {
        /* For the pages in memory, go through and see if they've been modified */
        if (pagedir_is_dirty (thread_current()->pagedir, (const void*)p->upage)) {
          /* If the number of bytes written isn't the same as expect, kill the thread */
          lock_acquire(&filesys_lock);
          if ((file_write_at(p->file, (const void*)(p->upage),(off_t)p->read_bytes,p->ofs) != (off_t)p->read_bytes) && kill_thread)
          {
            lock_release(&filesys_lock);
            thread_exit();
          }
          lock_release(&filesys_lock);
          page_table_remove (p, sup);
        }
      }
      else {
        lock_acquire(&filesys_lock);
        /* If the number of bytes written isn't the same as expect, kill the thread */
        if ((file_write_at(file, (const void*)p->upage,p->read_bytes,p->ofs) != (off_t)p->read_bytes) && kill_thread) 
        {
          lock_release(&filesys_lock);
          thread_exit();
        }
        page_table_remove (p, sup);
        
        lock_release(&filesys_lock);
      }
    }
    else if (p != NULL)
        page_table_remove(p,sup);
  }
  
  m->file->mmaped = false;  
  
  /* If the process has closed the file - actually close the file */
  if (m->file->closed) {
    lock_acquire(&filesys_lock);
    file_close(m->file);
    lock_release(&filesys_lock);
    
  }
  list_remove(&m->elem);
  free(m);
}


/* --- Syscall return methods ---*/

static void
syscall_return_int (uint32_t* eax, const int value)
{
  *eax = value;  
}

static void
syscall_return_pid_t (uint32_t* eax, const pid_t value)
{
  *eax = value;
}

static void
syscall_return_bool (uint32_t* eax, const bool value)
{
  *eax = value;
}

static void
syscall_return_uint (uint32_t* eax, const unsigned value)
{
  *eax = value;
}

static void
syscall_return_mapid_t (uint32_t* eax, const mapid_t value)
{
  *eax = value;
}
/* ---- End of return methods ---- */


/* Returns the file associated with the given file descriptor */
static struct file*
find_file(int fd)
{
  struct thread* t;
  struct list_elem* e;
  struct file* file;
 
  t = thread_current();
  for (e = list_begin (&t->process->open_files); e != list_end (&t->process->open_files);
     e = list_next (e)) 
    {
      file = list_entry (e, struct file, elem);
      if(file->fd == fd)
        return file; 
    }
  return NULL;
}

static struct mmap_file*
find_mmap (mapid_t id)
{
  struct process* p;
  struct list_elem* e;
  struct mmap_file* m;
 
  p = thread_current()->process;
  
  for (e = list_begin (&p->mmaped_files); e != list_end (&p->mmaped_files);
     e = list_next (e)) 
    {
      m = list_entry (e, struct mmap_file, elem);
      if(m->value == id)
        return m; 
    }
  return NULL;
}

/* Checks the buffer is safe by checking at each buffer + PGSIZE */
static void 
check_buffer_safety (const void* buffer, int size)
{
  int i;
  
  /* Check if buffer and if the end of the buffer are safe */
  if ( !is_safe_ptr(buffer) || !is_safe_ptr(buffer + size))
    thread_exit();
  
  /* Check if at each PGSIZE interval the buffer is safe */
  for(i = 1; i <= size / PGSIZE ; i++)
  {
    if( !is_safe_ptr(buffer + (i*PGSIZE)))
      thread_exit();
  }
}

/* Given the number of arguments, checks that they are all safe pointers*/
static void 
check_safe_ptr (const void* ptr, int no_args)
{
  int i;
  for(i = 0; i < no_args; i++)
    if (!is_safe_ptr(ptr + (i * sizeof(uint32_t))))
      thread_exit();
}

/* Checks that none of the pages have already been allocated */
static bool
check_pages (const void* addr, int size, struct sup_table* sup)
{
  int i;
  /* Check that we're not going to try and go inside the stack */
  if ((uint32_t*)MAX_STACK_ADDRESS < (uint32_t*)addr 
       || (uint32_t*)MAX_STACK_ADDRESS < (uint32_t*)addr + size)
    return false;
  
  for (i = 0; i <= size / PGSIZE ; i++) {
    if (page_find ((uint8_t*)addr + (i * PGSIZE), sup) != NULL) {
      return false;
    }
  }
    
  return true;
}