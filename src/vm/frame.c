#include "frame.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "vm/swap.h"

static struct frame** table;
static struct lock lock;

void
frame_init(int count)
{
  lock_init(&lock);
  lock_acquire(&lock);
  table = malloc(sizeof(struct frame*) * count);
  lock_release(&lock);
}

void
frame_add(unsigned int frame_index, struct page* sup_page)
{
  lock_acquire(&lock);
  ASSERT(table[frame_index] == NULL);
  table[frame_index]->sup_page = sup_page;
  lock_release(&lock);
}

void
frame_del(int frame_index)
{
  lock_acquire(&lock);
  ASSERT(table[frame_index] != NULL);
  free(table[frame_index]);
  lock_release(&lock);
}


void*
frame_get(enum palloc_flags flags, struct page* sup_page)
{
  void* kpage = palloc_get_page(flags);
  
  while(kpage == NULL)
  {
    swap_out(table[0]->sup_page);
    /* Swap out some page
    TODO: Search for least recently used clean page
    If no clean pages, then LRU dirty page */
    
    /* Try again */
    kpage = palloc_get_page(flags);
  }
  
  frame_add(page_to_frame_idx(kpage), sup_page);
  
  return kpage;
}