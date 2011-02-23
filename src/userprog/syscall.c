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
static int syscall_filesize(int fd);
static int syscall_read(int fd, void *buffer, unsigned int size);
static int syscall_write(int fd, const void *buffer, unsigned size);
static void syscall_seek(int fd, unsigned int position);
static unsigned int syscall_tell(int fd);
static void syscall_close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  unsigned int call_number = *(int*)(f->esp);

  void* argument_1, argument_2, argument_3;
  int* esp = (f->esp);
  
  switch(call_number)
  {
    case SYS_HALT: 
      syscall_halt(); 
      break;
    case SYS_EXIT:
      argument_1 = *(esp+1);
      syscall_exit(argument_1); 
      break;
    case SYS_EXEC:
      argument_1 = (esp + 1);
      syscall_exec((char*)argument_1); 
      break;
    case SYS_WAIT:
      argument_1 = *(esp + 1);
      syscall_wait((pid_t)argument_1); 
      break;
    case SYS_CREATE:
      argument_1 = esp + 1;
      argument_2 = esp + 2;
      syscall_create((char*)argument_1, (unsigned int)*(argument_2)); 
      break;
    case SYS_REMOVE: 
      argument_1 = esp + 1;
      syscall_remove((char*)argument_1); 
      break;
    case SYS_OPEN:
      argument_1 = esp + 1;
      syscall_open((char*)argument_1); 
      break;
    case SYS_FILESIZE:
      argument_1 = esp + 1;
      syscall_filesize(*(argument_1)); 
      break;
    case SYS_READ: 
      argument_1 = esp + 1;
      argument_2 = esp + 2;
      argument_3 = esp + 3;
      syscall_read(*(argument_1), (void*)argument_2, (unsigned int)*(argument_3)); 
      break;
    case SYS_WRITE: 
      argument_1 = esp + 1;
      argument_2 = esp + 2;
      argument_3 = esp + 3;
      syscall_write(*(argument_1), (void*)argument_2, (unsigned int)*(argument_3)); 
      break;
    case SYS_SEEK: 
      argument_1 = esp + 1;
      argument_2 = esp + 2;
      syscall_seek(*(argument_1), (unsigned int)*(argument_3)); 
      break;
    case SYS_TELL:
      argument_1 = esp + 1;
      syscall_tell(*(argument_1)); 
      break;
    case SYS_CLOSE: 
      argument_1 = esp + 1;
      syscall_close(*(argument_1)); 
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
syscall_write(int fd, const void *buffer, unsigned size)
{
  
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
