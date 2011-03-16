#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <bitmap.h>
#include <stdint.h>

/* This file will define a page struct*/

struct page {
  
  uint32_t* page;
  bool in_memory;
  
  void* disk; /* This is wrong - figure out wtf block devices are */
  
  struct hash_elem elem;

};

struct sup_page_table {
  
  struct hash page_table;
  
};


struct sup_page_table* page_table_init (void);

#endif