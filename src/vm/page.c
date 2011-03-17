#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"
#include <stdio.h>

static bool page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux);
static unsigned page_hash (const struct hash_elem* elem, void* aux);
static void page_free (struct hash_elem* e, void* aux);


bool
page_table_init (struct sup_table* sup) 
{
  bool success;
  success = false;
  //struct hash* hash;
  //printf("sizeof sup_table %d\n size of hash %d\n", sizeof(struct sup_table), sizeof(struct hash));
  
  //hash = malloc(sizeof(struct hash));
  
  //if (hash == NULL)
  //  return success;
  
  //sup->page_table = hash;
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
add_page (uint8_t* kpage, uint8_t* upage, bool writable, struct sup_table* table)
{
  struct page* page;
  
  page = malloc(sizeof(struct page));
  
  page->upage = upage;
  page->kpage = kpage;
  page->writable = writable;
  
  page_table_add(page, table);  
  
  return true;
}

/* Unsure if this works */
struct page*
page_find (uint8_t* upage, struct sup_table* sup)
{
  struct page* page;
  struct page* return_value;
  struct hash_elem* value;
  page = malloc(sizeof(struct page));
  
  page->upage = upage;
  
  value = hash_find(&sup->page_table, &page->elem);
  
  free(page);
  
  return_value = hash_entry(value, struct page, elem);
  return return_value;
}

void
page_table_destroy(struct sup_table* sup)
{
  hash_destroy(&sup->page_table, page_free);
  
  free(sup);  
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