#include "vm/page.h"
#include "userprog/process.h"
#include <hash.h>
#include "threads/malloc.h"
#include <stdio.h>
#include "threads/vaddr.h"


static bool page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux);
static unsigned page_hash (const struct hash_elem* elem, void* aux);
static void page_free (struct hash_elem* e, void* aux);

static void print_page (struct hash_elem* e, void* aux UNUSED);



bool
page_table_init (struct sup_table* sup) 
{
  bool success;
  success = hash_init(&sup->page_table, page_hash, page_less, NULL);
  return success;
}


bool
page_table_add (struct page* p, struct sup_table* table)
{
  bool success;
  success = false;
    
  if (hash_insert (&table->page_table, &p->elem) == NULL)
    success = true;
  
  return success;
}


bool
page_table_remove (struct page* p, struct sup_table* table)
{
  bool success;
  success = false;
  if (hash_delete (&table->page_table, &p->elem) != NULL)
    success = true;  
  
  return success;
}

struct page*
page_table_find (struct page* p, struct sup_table* table)
{
  struct hash_elem* elem;
  elem = hash_find (&table->page_table, &p->elem);
    
  return (hash_entry (elem, struct page, elem));
}

bool
add_page (uint8_t* upage, bool writable, struct sup_table* table)
{
  struct page* page;
  
  page = malloc(sizeof(struct page));
  
  page->upage = upage;
  page->writable = writable;
  
  page_table_add(page, table);  
  
  return true;
}

struct page*
page_find (uint8_t* upage, struct sup_table* sup)
{
  struct page page;
  struct page* return_value;
  struct hash_elem* value;
  
  
  page.upage = upage;
  
  value = hash_find(&sup->page_table, &page.elem);
  if (value == 0)
    return NULL;
  
  return_value = hash_entry(value, struct page, elem);

  return return_value;
}

uint32_t*
lookup_sup_page(struct process* process, const void* vaddr)
{
  
  struct sup_table* sup = process->sup_table;
  
  struct page* ptr = page_find((uint8_t*)vaddr, sup);
  if (ptr != NULL)
    return (uint32_t*)ptr->upage;
  
  return NULL;
  
}

void
page_table_destroy(struct sup_table* sup)
{
  hash_destroy(&sup->page_table, page_free);
  
  free(sup);  
}

void*
lower_page_bound (const void* vaddr) 
{
  return (void*)((uint32_t)vaddr - ((uint32_t)vaddr % PGSIZE));
}


/* Hash table functions */

static bool
page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux UNUSED)
{
  uint8_t* page_1 = hash_entry (p1, struct page, elem)->upage;
  uint8_t* page_2 = hash_entry (p2, struct page, elem)->upage;
  
  return page_1 < page_2;
}

static unsigned
page_hash (const struct hash_elem* elem, void* aux UNUSED)
{
  uint8_t* page_addr = hash_entry(elem, struct page, elem)->upage;
  
  return hash_int((uint32_t)page_addr);
}

static void
page_free (struct hash_elem* e, void* aux UNUSED)
{
  struct page* page = hash_entry(e, struct page, elem);
  free(page); 
  
}

/* Debug functions */

static void
print_page (struct hash_elem* e, void* aux UNUSED)
{
  struct page* page = hash_entry(e, struct page, elem);
  
  printf("Page addr = %X\n",page->upage);
    
}

void
debug_page_table (struct sup_table* sup)
{
  hash_apply (&sup->page_table,print_page);
}