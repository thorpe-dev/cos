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
      syscall_exit(*((int*)argument_1)); 
      break;
    case SYS_EXEC:
      syscall_exec((const char*)argument_1); 
      break;
    case SYS_WAIT:
      syscall_wait(*((pid_t*)argument_1)); 
      break;
    case SYS_CREATE:
      syscall_create((const char*)argument_1, *((unsigned int*)argument_2)); 
      break;
    case SYS_REMOVE: 
      syscall_remove((const char*)argument_1); 
      break;
    case SYS_OPEN:
      syscall_open((const char*)argument_1); 
      break;
    case SYS_FILESIZE:
      syscall_filesize(*((int*)argument_1)); 
      break;
    case SYS_READ: 
      syscall_read(*((int*)argument_1), argument_2, *((unsigned int*)argument_3)); 
      break;
    case SYS_WRITE: 
      syscall_write(*((int*)argument_1), (const void*)argument_2, *((unsigned int*)argument_3)); 
      break;
    case SYS_SEEK: 
      syscall_seek(*((int*)argument_1), *((unsigned int*)argument_3)); 
      break;
    case SYS_TELL:
      syscall_tell(*((int*)argument_1)); 
      break;
    case SYS_CLOSE: 
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
