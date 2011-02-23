#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_execute (const char *command);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* Process identifier - same as in lib/user/syscall.h */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

struct arg_elem
{
  char argument[128];
  int length;
  uint32_t* location;
  struct list_elem elem;
};

struct process
{
  pid_t pid;
  struct thread* thread;
  struct semaphore running;

  struct list children;
  struct list_elem elem;
};

#endif /* userprog/process.h */
