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

  printf("Thread %s made a syscall!\n", thread_current()->name);

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
  struct process* p;
  process_execute(command);
  // New process is at the head of the list of this process's children
  p = list_entry(list_front(&(thread_current()->process->children)), struct process, child_elem);

  sema_down(&p->load_complete);
  sema_up(&p->load_complete);

  syscall_return_int (eax, p->pid);
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
  
  success = filesys_create(file, initial_size);
  
  syscall_return_bool (eax, success);
}


static void 
syscall_remove(uint32_t* eax, const char *file)
{
  bool success = false;
  
  success = filesys_remove(file);
  
  syscall_return_bool (eax, success);
}


static void 
syscall_open(uint32_t* eax, const char *file_name)
{
  struct file* file_ptr;
  file_ptr = filesys_open(file_name);
  
  syscall_return_int (eax, 2);
}


/* UNIMPLEMENTED */
static void
syscall_write(uint32_t* eax, int fd, const void *buffer, unsigned int size)
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
      syscall_return_int (eax, size);
    }
  }
  
  else
    syscall_return_int (eax, size); 
}


static void 
syscall_filesize(uint32_t* eax, int fd) 
{
  syscall_return_int (eax, 0);
}

static void 
syscall_read(uint32_t* eax, int fd, void *buffer, unsigned int size)
{
  syscall_return_int (eax, 0);
}

static void 
syscall_tell (uint32_t* eax, int fd) 
{
  syscall_return_uint (eax, (unsigned int)0);
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

