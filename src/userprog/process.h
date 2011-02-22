#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct arg_elem
  {
    char argument[128];
    int argument_length;
    char* stack_pointer;
    struct list_elem elem;
  };

#endif /* userprog/process.h */
