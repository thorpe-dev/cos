#include "swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/palloc.h"

//TODO: Remove (debug)
#include <stdio.h>

static struct block* swap_area;
static struct bitmap* swap_state;
static struct lock lock;

static void write_out(block_sector_t sec, void* data);
static block_sector_t idx_to_sec(unsigned int swap_index);

/* TODO: When a process exits, free up any allocated swap space */


void
swap_init(void)
{
  swap_area = block_get_role(BLOCK_SWAP);

  swap_state = bitmap_create((block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
}


/* Checks if the page already has some swap space allocated.
   If so, checks the dirty bit and swaps out if necessary. 
   If not, finds some new space in swap and swaps out the page to it */
void
swap_out(struct page* page)
{
  unsigned int swap_page_idx;
  
  ASSERT(page->loaded);
  
  lock_acquire(&lock);
  
  
  /* No swap yet allocated - has not been swapped out before */
  if(page->swap_idx == NOT_YET_SWAPPED)
  {
    /* Scan for a single free page in swap block device */
    swap_page_idx = bitmap_scan_and_flip(swap_state, 0, 1, false);
    if(swap_page_idx == BITMAP_ERROR)
      PANIC("No space left on swap disk!\n");
    
    page->swap_idx = swap_page_idx;
    
    write_out(idx_to_sec(swap_page_idx), page->upage);
  }
  /* Has been swapped out before, but needs to be written back to disk */
  else if(pagedir_is_dirty(page->owner->pagedir, page->upage))
  {
    write_out(idx_to_sec(page->swap_idx), page->upage);
  }

  page->in_memory = false;
  // TODO: Remove page->upage from page directory
  
  palloc_free_page(page->upage);
  lock_release(&lock);
}

void*
swap_in(struct page* page)
{
  block_sector_t sec;
  void* data;
  void* swapped_to;

  ASSERT(page->loaded);
  ASSERT(page->owner == thread_current());
  ASSERT(!page->in_memory);
  
  lock_acquire(&lock);

  /* Set data to location of some free PGSIZE area in RAM */
  data = swapped_to = palloc_get_page(PAL_USER); //Here we want an arbitrary address
  
  sec = idx_to_sec(page->swap_idx);
  
  while(sec < PGSIZE/BLOCK_SECTOR_SIZE) {
    block_read(swap_area, sec++, data);
    data += BLOCK_SECTOR_SIZE;
  }
  
  lock_release(&lock);
  
  return swapped_to;
}

/* Writes one page from DATA, starting at sector SEC */
static void
write_out(block_sector_t sec, void* data)
{
  while(sec < PGSIZE/BLOCK_SECTOR_SIZE) {
    block_write(swap_area, sec++, data);
    data += BLOCK_SECTOR_SIZE;
  }
}

/* Converts a page index into a sector index.
A page index identifies a PGSIZE block in the swap area.
A sector index identifies a BLOCK_SECTOR_SIZE block in the swap area. */
static block_sector_t
idx_to_sec(unsigned int swap_idx)
{
  /* This doesn't handle pages smaller than disk sectors */
  ASSERT(PGSIZE > BLOCK_SECTOR_SIZE);
  /* Usually 8*swap_idx because PGSIZE=4096 and BLOCK_SECTOR_SIZE=512 */
  return swap_idx * PGSIZE / BLOCK_SECTOR_SIZE;
}