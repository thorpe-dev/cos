#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "filesys/off_t.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1

tid_t process_execute (const char *command);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void load_page(struct file *file, struct page* p);

bool install_page (void *upage, void *kpage, bool writable);


/* Process identifier - same as in lib/user/syscall.h */
typedef int pid_t;

/* Memory map identifier - same as in lib/user/syscall.h */
typedef int mapid_t;

#define PID_ERROR ((pid_t) -1)

struct arg_elem
{
  char* argument;         /* Argument */
  int length;             /* Length of argument (including \0) */
  char* location;         /* Location on stack */
  struct list_elem elem;  /* list_elem */
};

struct mmap_file
{
  mapid_t value;
  struct file* file;
  void* addr;
  size_t file_size;
  
};

struct process
{
  char* command;                    /* Command for use when loading process/thread */
  bool load_success;                /* Used with load_complete */
  struct semaphore load_complete;   /* Ensures load is complete before process_execute() finishes */
  int exit_status;                  /* Process exit status initialised to EXIT_FAILURE */
  struct semaphore exit_complete;   /* Used in process_wait() */
  pid_t pid;                        /* Process pid */
  struct list_elem child_elem;      /* So it can be made a child of another processes thread*/
  struct list open_files;           /* List of files the process has open */
  struct list mmaped_files;         /* List of memory mapped files */
  int next_fd;                      /* Used for generating file descriptors*/
  struct file* process_file;        /* The current process's executable */
  struct sup_table* sup_table;      /* Hash table of pages */
  uint8_t* heap_top;                /* Location of the top of the heap */
};


#endif /* userprog/process.h */
