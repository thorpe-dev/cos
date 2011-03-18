#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct frame {
  void* sup_page;
  // TODO: Add extra information for eviction
};

void frame_init(int count);
void frame_add(unsigned int frame_index, struct page* sup_page);
void frame_del(int frame_index);
void* frame_get(enum palloc_flags flags, struct page* sup_page);
