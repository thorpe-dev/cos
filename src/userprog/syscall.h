#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"

void syscall_init (void);
void un_map_file (struct mmap_file* m, bool kill_thread);


#endif /* userprog/syscall.h */