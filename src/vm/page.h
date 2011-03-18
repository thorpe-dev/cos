#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <bitmap.h>
#include <stdint.h>
#include "filesys/off_t.h"

/* This file will define a page struct*/

struct page {
  
  uint8_t* upage;       /* User virtual address of page*/
  off_t ofs;            /* Offset into process exec file which will be used to load actual pages - map them to kernel pages ??*/
//  uint8_t* kpage;       /* Kernel virtual address of page - will not be mapped to start with*/
  uint32_t read_bytes; /* Number of bytes that need to be read */
  uint32_t zero_bytes; /* Number of bytes that need to be zeroed */
  
  bool writable;        /* Whether the page is writable or not */
  bool loaded;          /* Has the page been loaded yet - will not be before being mapped to a kpage*/
  bool valid;       /* If the page has been loaded is it mapped to a frame or swap */
  
  struct thread* owner;
  uint32_t swap_idx;
  
  struct hash_elem elem;

};

struct sup_table {
  
  struct process* process;
  struct hash page_table;

  
};

bool page_table_init (struct sup_table* sup);
bool page_table_add (struct page* p, struct sup_table* table);
bool page_table_remove (struct page* p, struct sup_table* table);
struct page* page_table_find (struct page* p, struct sup_table* table);
struct page* add_page (uint8_t* upage, bool writable, struct sup_table* table);
struct page* page_find (uint8_t* upage, struct sup_table* sup);
uint32_t* lookup_sup_page (struct process* process, const void* vaddr);
void* lower_page_bound (const void* vaddr);
void load_buffer_pages(const void* buffer, unsigned int size);


void page_table_destroy(struct sup_table* sup);

void debug_page_table (struct sup_table* sup);




#endif