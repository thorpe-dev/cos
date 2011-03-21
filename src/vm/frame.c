#include "frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

//TODO: remove (debug)
#include <stdio.h>

static struct frame** table;
static unsigned int count;

static void frame_add(unsigned int frame_index, struct page* sup_page);
static void frame_del(unsigned int frame_index);

void
frame_init(int _count)
{
  unsigned int i;
  count = _count;
  table = malloc(sizeof(void*) * count);
  
  /* Initialise (struct frame) pointers */
  for(i=0;i<count;i++)
  {
    table[i] = NULL;
  }
}

static void
frame_add(unsigned int frame_index, struct page* sup_page)
{
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
  
  ASSERT(sup_page->upage != NULL);

  kpage = palloc_get_page(PAL_USER | flags);

  /* Evict if necessary */
  if(kpage == NULL)
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

        if(current_score >= best_score)
        {
          best = table[i];
          best_score = current_score;
        }
      }
    }
    
    swap_out(best->sup_page);
    /* Swap out some page */
    
    /* Try again */
    kpage = palloc_get_page(PAL_USER | flags);
  }
  
  frame_add(page_to_frame_idx(kpage), sup_page);
  
  /* Add to page directory */
  ASSERT(install_page(sup_page->upage, kpage, sup_page->writable));
  
  /* Set supplementary page "in physical memory" flag */
  sup_page->valid = true;
  
  return kpage;
}

/* Called from page_free to free a page which is in physical memory */
void
frame_free(struct page* sup_page)
{
  void* kpage;
  
  ASSERT(sup_page->valid);
  
  sup_page->valid = false;
  kpage = pagedir_get_page(sup_page->owner->pagedir, sup_page->upage);
  pagedir_clear_page(sup_page->owner->pagedir, sup_page->upage);
  palloc_free_page(kpage);
  frame_del(page_to_frame_idx(kpage));
}
