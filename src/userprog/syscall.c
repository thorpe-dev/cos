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

#define MAXCHAR 512

static void syscall_handler (struct intr_frame *);

static void syscall_halt(void);
static void syscall_exit(int status);
static void syscall_exec(uint32_t* eax, const char *command);
static void syscall_wait(uint32_t* eax, pid_t pid);
static void syscall_create(uint32_t* eax, const char *file, unsigned initial_size);
static void syscall_remove(uint32_t* eax, const char *file);
static void syscall_open(uint32_t* eax, const char *file_name);
static void syscall_filesize(uint32_t* eax, int fd);
static void syscall_read(uint32_t* eax, int fd, void *buffer, unsigned int size);
static void syscall_write(uint32_t* eax, int fd, const void *buffer, unsigned size);
static void syscall_seek(int fd, unsigned int position);
static void syscall_tell(uint32_t* eax, int fd);
static void syscall_close (int fd);

static void check_safe_ptr (const void *ptr, int no_args);

static void syscall_return_int (uint32_t* eax, const int value);
static void syscall_return_pid_t (uint32_t* eax, const pid_t value);
static void syscall_return_bool (uint32_t* eax, const bool value);
static void syscall_return_uint (uint32_t* eax, const unsigned value);

static struct open_file* get_file_fd (int fd);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  
  uint32_t* esp = (f->esp);
  uint32_t* eax = &(f->eax);
  unsigned int call_number = *esp;

  void* argument_1 = esp +     sizeof(uint32_t);
  void* argument_2 = esp + 2 * sizeof(uint32_t);
  void* argument_3 = esp + 3 * sizeof(uint32_t);
      
  
  switch(call_number)
  {
    case SYS_HALT: 
      syscall_halt(); 
      break;
      
    case SYS_EXIT:
      check_safe_ptr (esp, 1);
      syscall_exit(*((int*)argument_1)); 
      break;
      
    case SYS_EXEC:
      check_safe_ptr (esp, 1);
      syscall_exec(eax, (const char*)argument_1); 
      break;
      
    case SYS_WAIT:
      check_safe_ptr (esp, 1);
      syscall_wait(eax, *((pid_t*)argument_1)); 
      break;
      
    case SYS_CREATE:
      check_safe_ptr (esp, 2);
      syscall_create(eax, (const char*)argument_1, *((unsigned int*)argument_2)); 
      break;
      
    case SYS_REMOVE: 
      check_safe_ptr (esp, 1);
      syscall_remove(eax, (const char*)argument_1); 
      break;
      
    case SYS_OPEN:
      check_safe_ptr (esp, 1);
      syscall_open(eax, (const char*)argument_1); 
      break;
      
    case SYS_FILESIZE:
      check_safe_ptr (esp, 1);
      syscall_filesize(eax, *((int*)argument_1)); 
      break;
      
    case SYS_READ:
      check_safe_ptr (esp, 3);
      syscall_read(eax, *((int*)argument_1), argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_WRITE: 
      check_safe_ptr (esp, 3);
      syscall_write(eax, *((int*)argument_1), (const void*)argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_SEEK: 
      check_safe_ptr (esp, 2);
      syscall_seek(*((int*)argument_1), *((unsigned int*)argument_2)); 
      break;
      
    case SYS_TELL:
      check_safe_ptr (esp, 1);
      syscall_tell(eax, *((int*)argument_1)); 
      break;
      
    case SYS_CLOSE: 
      check_safe_ptr (esp, 1);
      syscall_close(*((int*)argument_1)); 
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
  printf("Exiting with status %d\n", status);
  // TODO: handle exit status
  printf("process_exit\n");
  process_exit();
  printf("thread_exit\n");
  thread_exit();
}

static void 
syscall_exec(uint32_t* eax, const char *command)
{
  struct semaphore load_success;
  struct thread* new_t;
  pid_t pid = -1;
    
  sema_init(&load_success, 0);
  
  pid = process_execute(command);
 
  syscall_return_int (eax, pid);
}

static void 
syscall_wait(uint32_t* eax, pid_t pid)
{
 /* ? */ 
 int status = -1;
 syscall_return_pid_t (eax, status);
}

static void 
syscall_create(uint32_t* eax, const char *file, unsigned int initial_size)
{
  bool success = false;
  
  lock_acquire(&filesys_lock);
  
  success = filesys_create(file, initial_size);
  
  lock_release(&filesys_lock);
  
  syscall_return_bool (eax, success);
}

/* Lock filesystem, remove file, unlock */
static void 
syscall_remove(uint32_t* eax, const char *file)
{
  bool success = false;
  
  lock_acquire(&filesys_lock);
  success = filesys_remove(file);
  lock_release(&filesys_lock);
  
  syscall_return_bool (eax, success);
}


static void 
syscall_open(uint32_t* eax, const char *file_name)
{
  struct file* file_ptr;
  struct thread* t = thread_current();
  struct open_file* open_file;
  
  int fd = t->process->next_fd;
  
  /* Lock filesystem, open file, unlock */
  lock_acquire(&filesys_lock);
  file_ptr = filesys_open(file_name);
  lock_release(&filesys_lock);
  

  /* If file not found, set eax to -1*/
  if (file_ptr == NULL) 
  {
    syscall_return_int(eax, -1);
  }
  else {
    /* Adds file to process' open_files list and returns fd */
    open_file = malloc(sizeof(struct open_file));
    if (open_file == NULL)
      syscall_return_int(eax, -1);
    else 
    {
      open_file->file = file_ptr;
      open_file->fd = fd;
      
      t->process->next_fd++;
      
      list_push_back(&t->process->open_files, &open_file->elem);
      
      syscall_return_int (eax, fd);
    }
  }
}


/* UNIMPLEMENTED */
static void
syscall_write(uint32_t* eax, int fd, const void *buffer, unsigned int size)
{
  int i;
  struct open_file* open_file;
  
  /* Writes to console, in blocks < maxchar */
  if (fd == 1) {
    for (i = size; i > 0; i -= MAXCHAR) {
      if (i < MAXCHAR)
        putbuf(buffer, i);

      else {
        putbuf(buffer, MAXCHAR);
        buffer += MAXCHAR;
      }
      syscall_return_int (eax, size);
    }
  }
  /* Write to file fd.*/
  else {
    open_file = get_file_fd ( fd );
    
    /* If fd is incorrect, return -1 */
    if (open_file == NULL)
      syscall_return_int(eax, -1);
    
    else {
      
      /* Lock filesystem, write to file, unlock */
      lock_acquire (&filesys_lock);
      i = (int)file_write (open_file->file, buffer, size);
      lock_release (&filesys_lock);
      syscall_return_int(eax,i);
    }
  }
}


static void 
syscall_filesize(uint32_t* eax, int fd) 
{
  int size;
  struct open_file* open_file;
  
  open_file = get_file_fd ( fd );
  
  lock_acquire(&filesys_lock);
  
  /* If fd is incorrect, return -1 */
  if (open_file == NULL)
    syscall_return_int(eax, -1);
  
  else 
  {
    size = (int) file_length(open_file->file);
  
    lock_release(&filesys_lock);
  
    syscall_return_int (eax, size);
  }
}

static void 
syscall_read(uint32_t* eax, int fd, void *buffer, unsigned int size)
{
  int read_size;
  struct open_file* open_file;
  
  open_file = get_file_fd ( fd );
  
  /* If fd is incorrect, return -1 */
  if (open_file == NULL)
    syscall_return_int(eax, -1);
  
  /* Lock filesystem, read file, unlock */
  else 
  {
    lock_acquire(&filesys_lock);
  
    read_size = (int) file_read(open_file->file, buffer, size);
  
    lock_release(&filesys_lock);
  
    syscall_return_int (eax, read_size);
  }
}

static void 
syscall_tell (uint32_t* eax, int fd) 
{
  int position;
  struct open_file* open_file;
  
  open_file = get_file_fd ( fd );
  
  /* If fd is incorrect, return -1 */
  if (open_file == NULL)
    syscall_return_uint (eax, -1);
  
  else 
  {
    /* Lock filesystem, read file position, unlock */
    lock_acquire(&filesys_lock);
  
    position = (int) file_tell(open_file->file);
  
    lock_release(&filesys_lock);
  
    syscall_return_uint (eax, position);
  }
}

static void 
syscall_seek (int fd, unsigned position) 
{
  struct open_file* open_file;
  
  open_file = get_file_fd ( fd );
  
  /* If fd is incorrect, exit */
  if (open_file == NULL)
    return;
  
  
  else
  {
    /* Lock filesystem, seek to position in file, unlock */
    lock_acquire(&filesys_lock);
    file_seek(open_file->file, (off_t)position);
    lock_release(&filesys_lock);
  }
}

static void 
syscall_close (int fd) 
{
  struct open_file* open_file;
  
  open_file = get_file_fd (fd);
  /* If fd is incorrect, exit */
  if (open_file == NULL)
    return;
  
  else
  {
    /* Lock filesystem, close file, unlock */
    lock_acquire(&filesys_lock);
  
    file_close(open_file->file);
  
    lock_release(&filesys_lock);
  }

}

/* Given the number of arguments, checks that they are all safe pointers*/
static void 
check_safe_ptr (const void *ptr, int no_args)
{
  int i;
  for(i = 0; i <= no_args; i++)
    if (!is_safe_ptr(ptr + (i * sizeof(uint32_t))))
      kill_current();
}

/* Syscall return methods */

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

static struct open_file*
get_file_fd (int fd)
{
  struct thread* t;
  struct list_elem* e;
  struct open_file* open_file;
 
  t = thread_current ();
  for (e = list_begin (&t->process->open_files); e != list_end (&t->process->open_files);
     e = list_next (e)) 
    {
      open_file = list_entry (e, struct open_file, elem);
      if ( open_file->fd == fd )
        return open_file; 
    }
  return NULL;
}