#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

static void syscall_halt(void);
static void syscall_exit(struct intr_frame *f);
static pid_t syscall_exec(struct intr_frame *f);
static void syscall_wait(struct intr_frame *f);
static void syscall_create(struct intr_frame *f);
static void syscall_remove(struct intr_frame *f);
static void syscall_open(struct intr_frame *f);
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
    case SYS_EXIT: syscall_exit(f->esp); break;
    case SYS_EXEC: syscall_exec(f->esp); break;
    /*case SYS_WAIT: syscall_wait(); break;
    case SYS_CREATE: syscall_create(); break;
    case SYS_REMOVE: syscall_remove(); break;
    case SYS_OPEN: syscall_open(); break;
    case SYS_FILESIZE syscall_filesize(); break;
    case SYS_READ: syscall_read(); break;
    case SYS_WRITE: syscall_write(); break;
    case SYS_SEEK: syscall_seek(); break;
    case SYS_TELL: syscall_tell(); break;
    case SYS_CLOSE: syscall_close(); break;*/
    default: 
      printf("Invalid syscall: %d\n", call_number);
  }
}


static void
syscall_halt(void) {
  printf("HALT!\n");
  shutdown_power_off();
}

static void
syscall_exit(struct intr_frame *f) {
  int* esp = (f->esp);
  int status = *(esp-1);
  printf("Exiting with status %d\n", status);
  // TODO: handle exit status
  printf("process_exit\n");
  process_exit();
  printf("thread_exit\n");
  thread_exit();
}

static pid_t syscall_exec(struct intr_frame *f) {
  char* command;
  
  
  
  // process_execute(command)
  // TODO: hell, synchronisation
}
