#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <bitmap.h>
#include <stdint.h>

/* This file will define a page struct*/

struct page {
  
  uint8_t* upage;
  uint8_t* kpage;
  uint32_t* read_bytes;
  uint32_t* zero_bytes;
  bool writable;
    
  bool in_memory;
  
  void* disk; /* This is wrong - figure out wtf block devices are */
  
  struct hash_elem elem;

};

struct sup_table {
  
  struct hash page_table;
  struct process* page_owner;

  
};


struct sup_table* page_table_init (void);
bool page_table_add (struct page* p, struct sup_table* table);
bool page_table_remove (struct page* p, struct sup_table* table);
struct page* page_table_find (struct page* p, struct sup_table* table);
bool add_page (uint8_t* kpage, uint8_t* upage, bool writable, struct sup_table* table);
struct page* page_find (uint8_t* upage, struct sup_table* sup);




#endif