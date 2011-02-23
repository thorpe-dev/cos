#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "process.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"

#define MAXCHAR 512

static void syscall_handler (struct intr_frame *);

static void syscall_halt(void);
static void syscall_exit(int status);
static pid_t syscall_exec(const char *command);
static void syscall_wait(pid_t pid);
static bool syscall_create(const char *file, unsigned initial_size);
static bool syscall_remove(const char *file);
static int syscall_open(const char *file_name);
static int syscall_filesize(int fd);
static int syscall_read(int fd, void *buffer, unsigned int size);
static int syscall_write(int fd, const void *buffer, unsigned size);
static void syscall_seek(int fd, unsigned int position);
static unsigned int syscall_tell(int fd);
static void syscall_close (int fd);

static void check_safe_ptr (const void *ptr, int no_args);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  
  uint32_t* esp = (f->esp);
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
      syscall_exec((const char*)argument_1); 
      break;
    case SYS_WAIT:
      check_safe_ptr (esp, 1);
      syscall_wait(*((pid_t*)argument_1)); 
      break;
    case SYS_CREATE:
      check_safe_ptr (esp, 2);
      syscall_create((const char*)argument_1, *((unsigned int*)argument_2)); 
      break;
    case SYS_REMOVE: 
      check_safe_ptr (esp, 1);
      syscall_remove((const char*)argument_1); 
      break;
    case SYS_OPEN:
      check_safe_ptr (esp, 1);
      syscall_open((const char*)argument_1); 
      break;
    case SYS_FILESIZE:
      check_safe_ptr (esp, 1);
      syscall_filesize(*((int*)argument_1)); 
      break;
    case SYS_READ:
      check_safe_ptr (esp, 3);
      syscall_read(*((int*)argument_1), argument_2, *((unsigned int*)argument_3)); 
      break;
    case SYS_WRITE: 
      check_safe_ptr (esp, 3);
      syscall_write(*((int*)argument_1), (const void*)argument_2, *((unsigned int*)argument_3)); 
      break;
    case SYS_SEEK: 
      check_safe_ptr (esp, 2);
      syscall_seek(*((int*)argument_1), *((unsigned int*)argument_2)); 
      break;
    case SYS_TELL:
      check_safe_ptr (esp, 1);
      syscall_tell(*((int*)argument_1)); 
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

static pid_t 
syscall_exec(const char *command)
{
  pid_t pid = -1;
  
  pid = process_execute(command);
  // TODO: hell, synchronisation
  
  return pid;
}

static void 
syscall_wait(pid_t pid)
{
 /* ? */ 
}

static bool 
syscall_create(const char *file, unsigned int initial_size)
{
  bool success = false;
  
  success = filesys_create(file, initial_size);
  
  return success;
}


static bool 
syscall_remove(const char *file)
{
  bool success = false;
  
  success = filesys_remove(file);
  
  return success;
}


static int 
syscall_open(const char *file_name)
{
  struct file* file_ptr;
  file_ptr = filesys_open(file_name);
  
  return 2;
}


/* UNIMPLEMENTED */
static int
syscall_write(int fd, const void *buffer, unsigned int size)
{
  int i;
  if (fd == 1) {
    for (i = size; i > 0; i -= MAXCHAR) {
      if (i < MAXCHAR)
        putbuf(buffer, i);

      else {
        putbuf(buffer, MAXCHAR);
        buffer += MAXCHAR;
      }
      return size;
    }
  }
  
  else
    return size; 
}


static int 
syscall_filesize(int fd) 
{
  return 0;
}

static int 
syscall_read(int fd, void *buffer, unsigned int size)
{
  return 0;
}

static unsigned int 
syscall_tell (int fd) 
{
  return 0;
}

static void 
syscall_seek (int fd, unsigned position) 
{
  /* ? */ 
}

static void 
syscall_close (int fd) 
{

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