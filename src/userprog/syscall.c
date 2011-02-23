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
static void syscall_exit(struct intr_frame *f);
static pid_t syscall_exec(struct intr_frame *f);
static void syscall_wait(struct intr_frame *f);
static bool syscall_create(struct intr_frame *f);
static bool syscall_remove(struct intr_frame *f);
static int syscall_open(struct intr_frame *f);
static void syscall_filesize(struct intr_frame *f);
static void syscall_read(struct intr_frame *f);
static void syscall_write(struct intr_frame *f);
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
/*    case SYS_OPEN: syscall_open(f); break;
    case SYS_FILESIZE syscall_filesize(f); break;
    case SYS_READ: syscall_read(f); break;
    case SYS_WRITE: syscall_write(f); break;
    case SYS_SEEK: syscall_seek(f); break;
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
syscall_exit(struct intr_frame *f) 
{
  int* esp = (f->esp);
  /* esp + 1 because it should be above esp in the stack */
  int status = *(esp+1);
  printf("Exiting with status %d\n", status);
  // TODO: handle exit status
  printf("process_exit\n");
  process_exit();
  printf("thread_exit\n");
  thread_exit();
}

static pid_t 
syscall_exec(struct intr_frame *f)
{
  char* command;
  int* esp = (f->esp);
  pid_t pid = -1;
  
  command = *(((char*)esp)+1);
  
   
  
  pid = process_execute(command);
  // TODO: hell, synchronisation
  
  return pid;
}

static void 
syscall_wait(struct intr_frame *f)
{
  int* esp = (f->esp);
  
  pid_t pid = *(esp+1);
  
}

static bool 
syscall_create(struct intr_frame *f)
{
  int* esp = (f->esp);
  
  char* file_name = *(((char*)esp) + 1);
  
  int file_size = *(esp + 2);
  bool success = false;
  
  success = filesys_create(file_name, file_size);
  
  return success;
}


static bool 
syscall_remove(struct intr_frame *f)
{
  int* esp = (f->esp);
  
  char* file_name = *(((char*)esp) + 1);
  
  bool success = false;
  
  success = filesys_remove(file_name);
  
  return success;
}


static int 
syscall_open(struct intr_frame *f)
{
  int* esp = (f->esp);
  
  char* file_name = *(((char*)esp) + 1);
  struct file* file_ptr;
  file_ptr = filesys_open(file_name);
  
  return 2;
}