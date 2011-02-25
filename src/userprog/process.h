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
  /* Command for use when loading process/thread */
  char* command;
  /* Initialised to false */
  bool load_success;
  /* Initialised to 0 */
  struct semaphore load_complete;
  /* Initialised to EXIT_FAILURE */
  int exit_status;
  /* Initialised to 0 */
  struct semaphore exit_complete;
  pid_t pid;
  struct list children;
  struct list_elem child_elem;
  /* Members below here are only initialised upon successful thread creation */
  struct thread* thread;

  struct file* process_file;
  struct list open_files;
  int next_fd;
};

/* Struct used for keeping track of opened files */
struct open_file
{
  struct file* file;
  
  int fd;
  
  struct list_elem elem;
};


#endif /* userprog/process.h */
