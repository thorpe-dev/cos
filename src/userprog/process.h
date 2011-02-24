#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1

tid_t process_execute (const char *command);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

void kill_current(void);

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
  char* command; //Command for use when loading process/thread
  bool load_success; //Initialised to false
  struct semaphore load_complete; //Initialised to 0
  int exit_status; //Initialised to EXIT_FAILURE
  struct semaphore exit_complete; //Initialised to 0
  pid_t pid;
  struct list children;
  struct list_elem child_elem;
  // Members below here are only initialised upon successful thread creation
  struct thread* thread;
};

#endif /* userprog/process.h */
