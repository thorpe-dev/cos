#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "filesys/filesys.h"
#include "filesys/file.h"


//TODO: Remove (debug)
#include <stdio.h>

static struct block* swap_area;
static struct bitmap* swap_state;
static struct lock lock;

static void write_out(block_sector_t sec, void* data);
static block_sector_t idx_to_sec(unsigned int swap_index);

void
swap_init(void)
{
  swap_area = block_get_role(BLOCK_SWAP);

  swap_state = bitmap_create((block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
  
  lock_init(&lock);
}


/* Checks if the page already has some swap space allocated.
   If so, checks the dirty bit and swaps out if necessary. 
   If not, finds some new space in swap and swaps out the page to it */
void
swap_out(struct page* sup_page)
{
  unsigned int swap_page_idx;

  lock_acquire(&lock);
  
  /* No swap yet allocated - has not been swapped out before */
  if(sup_page->swap_idx == NOT_YET_SWAPPED)
  {
    /* For a file loaded into RAM but not written to, we just pretend
       that it was never loaded */
    if (sup_page->file != NULL && !sup_page->writable)
      sup_page->loaded = false;
    
    else if (sup_page->file != NULL && !pagedir_is_dirty(sup_page->owner->pagedir, sup_page->upage))
      sup_page->loaded = false;
    
    /* If a file is memory mapped and has been edited - write back to filesys, not swap */
    else if (     sup_page->file != NULL
              &&  sup_page->file != sup_page->owner->process->process_file
              &&  pagedir_is_dirty(sup_page->owner->pagedir, sup_page->upage))
    {
      lock_acquire(&filesys_lock);
      file_write_at(sup_page->file, (const void*)(sup_page->upage),(off_t)sup_page->read_bytes,sup_page->ofs);
      lock_release(&filesys_lock);
      sup_page->loaded = false;
    }
    else 
    {
    /* Scan for a single free page in swap block device */
    swap_page_idx = bitmap_scan_and_flip(swap_state, 0, 1, false);
    if(swap_page_idx == BITMAP_ERROR)
      PANIC("No space left on swap disk!\n");
    
    sup_page->swap_idx = swap_page_idx;
    
    write_out(idx_to_sec(swap_page_idx), sup_page->upage);
    }
  }
  /* Has been swapped out before, but needs to be written back to disk */
  else if(pagedir_is_dirty(sup_page->owner->pagedir, sup_page->upage))
  {
    write_out(idx_to_sec(sup_page->swap_idx), sup_page->upage);
  }
  
  frame_free(sup_page);
  
  lock_release(&lock);
}

void
swap_in(struct page* sup_page)
{
  block_sector_t sec;
  uint32_t swap_idx;
  void* kpage;
  void* data;
  unsigned int i;
  
  //ASSERT(sup_page->owner == thread_current());
  ASSERT(!sup_page->valid);

  swap_idx = sup_page->swap_idx;
  /* Set kpage to location of some free PGSIZE area in RAM */
  kpage = frame_get(PAL_USER, sup_page);
  // frame_get clobbers sup_page->swap_idx - we must preserve it
  sup_page->swap_idx = swap_idx;

  lock_acquire(&lock);
  
  install_page(sup_page->upage, kpage, sup_page->writable);
   
  sec = idx_to_sec(swap_idx);
  data = sup_page->upage;  
  for(i=0;i < PGSIZE/BLOCK_SECTOR_SIZE;i++) {
    block_read(swap_area, sec+i, data+i*BLOCK_SECTOR_SIZE);
  }
  
  lock_release(&lock);
}

void
swap_free(struct page* sup_page)
{
  lock_acquire(&lock);
  ASSERT(sup_page->loaded);
  ASSERT(!sup_page->valid);
  
  bitmap_flip(swap_state, sup_page->swap_idx);
  
  lock_release(&lock);
}

/* Writes one page from DATA, starting at sector SEC */
static void
write_out(block_sector_t sec, void* data)
{
  unsigned int i;
  for(i=0;i < PGSIZE/BLOCK_SECTOR_SIZE;i++) {
    block_write(swap_area, sec+i, data+i*BLOCK_SECTOR_SIZE);
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