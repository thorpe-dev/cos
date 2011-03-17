#include "swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>

//TODO: Remove (debug)
#include <stdio.h>

static struct block* swap_area;
static struct bitmap* swap_state;

void
swap_init(void)
{
  struct block* swap_area = block_get_role(BLOCK_SWAP);

  swap_state = bitmap_create((block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
  printf("Created swap state bitmap, size %d\n", (block_size(swap_area) * BLOCK_SECTOR_SIZE) / PGSIZE);
}


/* Swaps the given block out of memory onto the disk, 
returning its location on disk */
void*
swap_out(struct page* page) {
  
}

uint8_t*
swap_in (struct page* page) {
  
 return NULL; 
}