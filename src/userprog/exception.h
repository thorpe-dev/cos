#ifndef USERPROG_EXCEPTION_H
#define USERPROG_EXCEPTION_H

/* Page fault error code bits that describe the cause of the exception.  */
#define PF_P 0x1    /* 0: not-present page. 1: access rights violation. */
#define PF_W 0x2    /* 0: read, 1: write. */
#define PF_U 0x4    /* 0: kernel, 1: user process. */
#define MAX_STACK_SIZE (1024*1024)  /* 1MB is the maxium stacksize */
#define MAX_STACK_ADDRESS (PHYS_BASE - MAX_STACK_SIZE) /* Lowest point the stack can grow to */

void exception_init (void);
void exception_print_stats (void);

#endif /* userprog/exception.h */
