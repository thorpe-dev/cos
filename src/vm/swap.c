#include "swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

//TODO: Remove (debug)
#include <stdio.h>

static struct block* swap_area;
static struct bitmap* swap_state;
static struct lock lock;

void
swap_init(void)
{
  swap_area = block_get_role(BLOCK_SWAP);

  swap_state = bitmap_create((block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
  printf("Created swap state bitmap, size %d\n", (block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
}


/* Checks if the page already has some swap space allocated.
   If so, checks the dirty bit and swaps out if necessary. 
   If not, finds some new space in swap and swaps out the page to it */
void
swap_out(struct page* page)
{
  unsigned int swap_page_idx;
  block_sector_t sec;
  void* ptr;
  
  lock_acquire(&lock);
  
  ptr = page->upage;
  
  /* No swap yet allocated - has not been swapped out before */
  if(page->swap_idx == NOT_YET_SWAPPED)
  {
    /* Scan for a single free page in swap block device */
    swap_page_idx = bitmap_scan_and_flip(swap_state, 0, 1, false);
    if(swap_page_idx == BITMAP_ERROR)
      PANIC("No space left on swap disk!\n");
    
    sec = swap_page_idx * PGSIZE / BLOCK_SECTOR_SIZE;
    while(sec < PGSIZE/BLOCK_SECTOR_SIZE) {
      block_write(swap_area, sec++, ptr);
      ptr += BLOCK_SECTOR_SIZE;
    }
    
    page->in_memory = false;
  }
  /* Has been swapped out before, but needs to be written back to disk */
  else if(pagedir_is_dirty(page->owner->pagedir, page->upage))
  {
    sec = page->swap_idx * PGSIZE / BLOCK_SECTOR_SIZE;
    while(sec < PGSIZE/BLOCK_SECTOR_SIZE) {
      block_write(swap_area, sec++, ptr);
      ptr += BLOCK_SECTOR_SIZE;
    }
  }
  
  lock_release(&lock);
}

void*
swap_in (struct page* page)
{
 return NULL; 
}