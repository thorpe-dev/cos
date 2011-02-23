#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "process.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

static void syscall_halt(void);
static void syscall_exit(int status);
static pid_t syscall_exec(const char *command);
static void syscall_wait(pid_t pid);
static bool syscall_create(const char *file, unsigned initial_size);
static bool syscall_remove(const char *file);
static int syscall_open(const char *file_name);
static void syscall_filesize(struct intr_frame *f);
static void syscall_read(struct intr_frame *f);
static int syscall_write(int fd, const void *buffer, unsigned size);
static void syscall_seek(struct intr_frame *f);
static void syscall_tell(struct intr_frame *f);
static void syscall_close(struct intr_frame *f);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  unsigned int call_number = *(int*)(f->esp);

  switch(call_number)
  {
    case SYS_HALT: syscall_halt(); break;
    case SYS_EXIT: syscall_exit(f); break;
    case SYS_EXEC: syscall_exec(f); break;
    case SYS_WAIT: syscall_wait(f); break;
    case SYS_CREATE: syscall_create(f); break;
    case SYS_REMOVE: syscall_remove(f); break;
/*  case SYS_OPEN: syscall_open(f); break;
    case SYS_FILESIZE syscall_filesize(f); break;
    case SYS_READ: syscall_read(f); break;*/
    case SYS_WRITE: syscall_write(f); break;
   /* case SYS_SEEK: syscall_seek(f); break;
    case SYS_TELL: syscall_tell(f); break;
    case SYS_CLOSE: syscall_close(f); break;*/
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
  int* esp = (f->esp);
  pid_t pid = -1;
  
  command = (char*)(((int)esp) + 1);
  
   
  
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
syscall_create(const char *file, unsigned initial_size)
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

static int
syscall_write(int fd, const void *buffer, unsigned size)
{
  
 return size; 
}