#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"

static bool page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux);
static unsigned page_hash (const struct hash_elem* elem, void* aux);


void
page_table_init (struct sup_table* sup) 
{
  sup = malloc(sizeof(struct sup_table));
  if (sup != NULL) {
    hash_init(&sup->page_table, page_hash, page_less, NULL);
  }
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

static bool
page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux UNUSED)
{
  uint32_t* page_1 = hash_entry (p1, struct page, elem)->upage;
  uint32_t* page_2 = hash_entry (p2, struct page, elem)->upage;
  
  return page_1 < page_2;
}

static unsigned
page_hash (const struct hash_elem* elem, void* aux UNUSED)
{
  uint32_t* page_addr = hash_entry(elem, struct page, elem)->upage;
  
  return hash_int((uint32_t)page_addr);  
}