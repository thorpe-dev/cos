#include "vm/page.h"
#include "threads/malloc.h"

static bool page_table_add (struct page* p, struct sup_page_table* table);
static void page_table_remove (struct page* p, struct sup_page_table* table);

struct sup_page_table*
page_table_init (void) 
{
  struct sup_page_table* ptr;
  
  ptr = malloc(sizeof(struct sup_page_table));
  
  return ptr;
}


static bool
page_table_add (struct page* p, struct sup_page_table* table)
{
  bool success;
  struct list* list;
  success = false;
  
  list = find_bucket (table->page_table, p->elem);
  if (list != NULL) {
    insert_elem (table->page_table, list, p->elem);
    success = true;
  }
  
  
  return success;
}


static void
page_table_remove (struct page* p, struct sup_page_table* table)
{
  remove_elem (table->page_table, p->elem);  
}