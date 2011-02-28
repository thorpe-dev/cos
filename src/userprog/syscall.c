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

#define MAXCHAR 512

static void syscall_handler (struct intr_frame *);

static void syscall_halt  (void);
static void syscall_exit  (int status);
static void syscall_exec  (uint32_t* eax, const char* command);
static void syscall_wait  (uint32_t* eax, pid_t pid);
static void syscall_create(uint32_t* eax, const char* file, unsigned initial_size);
static void syscall_remove(uint32_t* eax, const char* file);
static void syscall_open  (uint32_t* eax, const char* file_name);
static void syscall_filesize(uint32_t* eax, int fd);
static void syscall_read  (uint32_t* eax, int fd, void* buffer, unsigned int size);
static void syscall_write (uint32_t* eax, int fd, const void* buffer, unsigned size);
static void syscall_seek  (int fd, unsigned int position);
static void syscall_tell  (uint32_t* eax, int fd);
static void syscall_close (int fd);

static void check_safe_ptr (const void* ptr, int no_args);
static void check_buffer_safety (const void* buffer, int size);

static void syscall_return_int    (uint32_t* eax, const int value);
static void syscall_return_pid_t  (uint32_t* eax, const pid_t value);
static void syscall_return_bool   (uint32_t* eax, const bool value);
static void syscall_return_uint   (uint32_t* eax, const unsigned value);

static struct file* find_file (int fd);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  
  uint32_t* esp = f->esp;
  uint32_t* eax = &f->eax;
  unsigned int call_number = 0xDEADBEEF;
  
  check_safe_ptr(esp, 0);
  call_number = *esp;

  void* argument_1 = (void*)(esp + 1);
  void* argument_2 = (void*)(esp + 2);
  void* argument_3 = (void*)(esp + 3);

  switch(call_number)
  {
    case SYS_HALT: 
      syscall_halt(); 
      break;
      
    case SYS_EXIT:
      check_safe_ptr (esp, 1);
      syscall_exit(*(int*)argument_1); 
      break;
      
    case SYS_EXEC:
      check_safe_ptr (esp, 1);
      syscall_exec(eax, *(const char**)argument_1); 
      break;
      
    case SYS_WAIT:
      check_safe_ptr (esp, 1);
      syscall_wait(eax, *((pid_t*)argument_1)); 
      break;
      
    case SYS_CREATE:
      check_safe_ptr (esp, 2);
      syscall_create(eax, *(const char**)argument_1, *(unsigned int*)argument_2); 
      break;
      
    case SYS_REMOVE: 
      check_safe_ptr (esp, 1);
      syscall_remove(eax, *(const char**)argument_1); 
      break;
      
    case SYS_OPEN:
      check_safe_ptr (esp, 1);
      syscall_open(eax, *(const char**)argument_1); 
      break;
      
    case SYS_FILESIZE:
      check_safe_ptr (esp, 1);
      syscall_filesize(eax, *(int*)argument_1); 
      break;
      
    case SYS_READ:
      check_safe_ptr (esp, 3);
      syscall_read(eax, *(int*)argument_1, *(void**)argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_WRITE: 
      check_safe_ptr (esp, 3);
      syscall_write(eax, *(int*)argument_1, *(void**)argument_2, *((unsigned int*)argument_3)); 
      break;
      
    case SYS_SEEK: 
      check_safe_ptr (esp, 2);
      syscall_seek(*(int*)argument_1, *(unsigned int*)argument_2); 
      break;
      
    case SYS_TELL:
      check_safe_ptr (esp, 1);
      syscall_tell(eax, *(int*)argument_1); 
      break;
      
    case SYS_CLOSE: 
      check_safe_ptr (esp, 1);
      syscall_close(*(int*)argument_1); 
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
syscall_read(uint32_t* eax, int fd, void *buffer, unsigned int size)
{
  
  check_buffer_safety(buffer, size);
  
  int read_size;
  struct file* file;
  
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
    file_close(file);
    lock_release(&filesys_lock);
    
  }

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

/* Checks the buffer is safe by checking at each buffer + PGSIZE */
static void 
check_buffer_safety (const void* buffer, int size)
{
  int i;
  
  /* Check if buffer and if the end of the buffer are safe */
  if ( !is_safe_ptr(buffer) || !is_safe_ptr(buffer + size))
    thread_exit();
  
  /* Check if at each PGSIZE interval the buffer is safe */
  for(i = 0; i > size / PGSIZE ; i++)
  {
    if( !is_safe_ptr(buffer + (i*PGSIZE)))
      thread_exit();
  }
  
}

/* Given the number of arguments, checks that they are all safe pointers*/
static void 
check_safe_ptr (const void *ptr, int no_args)
{
  int i;
  for(i = 0; i <= no_args; i++){
    if (!is_safe_ptr(ptr + (i * sizeof(uint32_t))))
      thread_exit();
  }
}
