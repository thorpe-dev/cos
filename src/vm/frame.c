#include "frame.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"

//TODO: remove (debug)
#include <stdio.h>

static struct frame* table;
static struct lock lock;

static void frame_add(unsigned int frame_index, struct page* sup_page);
static void frame_del(unsigned int frame_index);

void
frame_init(int count)
{
  lock_init(&lock);
  lock_acquire(&lock);
  table = malloc(sizeof(struct frame) * count);
  lock_release(&lock);
}

static void
frame_add(unsigned int frame_index, struct page* sup_page)
{
  ASSERT(table[frame_index].sup_page == NULL);
  ASSERT(sup_page->loaded);
  table[frame_index].sup_page = sup_page;
}

static void
frame_del(unsigned int frame_index)
{
  ASSERT(table[frame_index].sup_page != NULL);
  table[frame_index].sup_page = NULL;
}


void*
frame_get(enum palloc_flags flags, struct page* sup_page)
{
  void* kpage;
  struct page* candidate;

  lock_acquire(&lock);

  kpage = palloc_get_page(PAL_USER | flags);

  while(kpage == NULL)
  {
    candidate = table[0].sup_page;
    ASSERT(candidate->loaded);
    swap_out(candidate);
    /* Swap out some page
    TODO: Search for least recently used clean page
    If no clean pages, then LRU dirty page */
    
    /* Try again */
    kpage = palloc_get_page(flags);
  }
  
  frame_add(page_to_frame_idx(kpage), sup_page);
  
  lock_release(&lock);
  
  return kpage;
}

//TODO: Finish
/*void*
frame_get_and_map(enum palloc_flags flags, struct page* sup_page)
{
  
}*/

void
frame_free(struct page* sup_page)
{
  void* kpage;
  
  ASSERT(sup_page->loaded);
  ASSERT(sup_page->valid);
  
  kpage = pagedir_get_page(sup_page->owner->pagedir, sup_page->upage);
  pagedir_clear_page(sup_page->owner->pagedir, sup_page->upage);
  palloc_free_page(kpage);
  frame_del(page_to_frame_idx(kpage));
}
