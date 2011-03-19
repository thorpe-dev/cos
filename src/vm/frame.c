#include "frame.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"

//TODO: remove (debug)
#include <stdio.h>

static struct frame** table;
static struct lock lock;
static unsigned int count;

static void frame_add(unsigned int frame_index, struct page* sup_page);
static void frame_del(unsigned int frame_index);

void
frame_init(int _count)
{
  lock_init(&lock);
  count = _count;
  table = malloc(sizeof(void*) * count);
  
}

static void
frame_add(unsigned int frame_index, struct page* sup_page)
{
  ASSERT(sup_page->valid);
  ASSERT(table[frame_index] == NULL);
  table[frame_index] = malloc(sizeof(struct frame));
  table[frame_index]->sup_page = sup_page;
}

static void
frame_del(unsigned int frame_index)
{
  ASSERT(table[frame_index] != NULL);
  free(table[frame_index]);
  table[frame_index] = NULL;
}


void*
frame_get(enum palloc_flags flags, struct page* sup_page)
{
  void* kpage;
  struct frame* best = NULL;
  int best_score = 0;
  int current_score = 0;
  uint32_t* pd;
  unsigned int i;
  bool accessed, dirty;
  
  lock_acquire(&lock);

  kpage = palloc_get_page(PAL_USER | flags);

  while(kpage == NULL)
  {
    for(i=0; i<count; i++)
    {
      if(table[i] != NULL)
      {
        pd = table[i]->sup_page->owner->pagedir;
        accessed = pagedir_is_accessed(pd, table[i]->sup_page->upage);
        dirty = pagedir_is_dirty(pd, table[i]->sup_page->upage);
        
        if(!accessed && !dirty)
          current_score = 4;
        if(accessed && !dirty)
          current_score = 3;
        if(!accessed && dirty)
          current_score = 2;
        else
          current_score = 1;

        if(current_score > best_score)
        {
          best = table[i];
          best_score = current_score;
        }
      }
    }
    
    lock_release(&lock);
    swap_out(best->sup_page);
    lock_acquire(&lock);
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
  
  lock_acquire(&lock);
  
  ASSERT(sup_page->valid);
  
  kpage = pagedir_get_page(sup_page->owner->pagedir, sup_page->upage);
  pagedir_clear_page(sup_page->owner->pagedir, sup_page->upage);
  palloc_free_page(kpage);
  frame_del(page_to_frame_idx(kpage));
  
  lock_release(&lock);
}
