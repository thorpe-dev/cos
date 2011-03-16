#include "frame.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

// TODO: Remove (debug)
#include <stdio.h>

static struct frame* table;
static struct lock lock;

void
frame_init(int count)
{
  lock_init(&lock);
  lock_acquire(&lock);
  table = malloc(sizeof(struct frame) * count);
  lock_release(&lock);
}

// TODO: Handle page_location == NULL (no frames free)
void
frame_add(int frame_index, void* page_location, int count)
{
  if(page_location == NULL)
    PANIC("No frames free!\n");


  lock_acquire(&lock);
  for(count--;count>=0;count--)
  {
    printf("Frame Table: Adding frame %d\n", frame_index+count);
    table[frame_index+count].page_location = (page_location + count*PGSIZE);
  }
  lock_release(&lock);
}

void
frame_del(int frame_index, int count)
{
  lock_acquire(&lock);
  for(count--;count>=0;count--)
  {
    printf("Frame Table: Removing frame %d\n", frame_index+count);
    table[frame_index+count].page_location = NULL;
  }
  lock_release(&lock);
}
