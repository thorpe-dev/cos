#include "vm/page.h"
#include "userprog/process.h"
#include <hash.h>
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/swap.h"

//TODO: Remove
#include <stdio.h>

static bool page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux);
static unsigned page_hash (const struct hash_elem* elem, void* aux);
static void page_destroy (struct hash_elem* e, void* aux);
static void page_copy (struct hash_elem* e, void* sup_table);


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
  if (success)
    free(p);
  
  return success;
}

bool
page_table_empty (struct sup_table* table)
{
  return hash_empty (&table->page_table);
}

struct page*
page_table_find (struct page* p, struct sup_table* table)
{
  struct hash_elem* elem;
  elem = hash_find (&table->page_table, &p->elem);
    
  return (hash_entry (elem, struct page, elem));
}

struct page*
add_page (uint8_t* upage, bool writable)
{
  struct page* page;
  
  page = malloc(sizeof(struct page));
  
  page->upage = upage;
  page->writable = writable;
  page->owner = thread_current();
  
  page_table_add(page, thread_current()->process->sup_table);  
  
  return page;
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
  hash_destroy(&sup->page_table, page_destroy);
  free(sup);  
}

void*
lower_page_bound (const void* vaddr) 
{
  return (void*)((uint32_t)vaddr - ((uint32_t)vaddr % PGSIZE));
}

void
load_buffer_pages(const void* buffer, unsigned int size)
{
  unsigned int i;
  uint8_t* addr;
  struct page* p;
  uint8_t* round_buffer = lower_page_bound(buffer);
   
  for(i = 0; i <= size / PGSIZE ; i++)
  {
    addr = round_buffer + (i*PGSIZE);
    p = page_find (addr,thread_current()->process->sup_table);
    if (p != NULL && !p->loaded)
      load_page (p);
  }
  
  /* If buffer is right at a page boundary, might miss a page to be loaded - check that it is loaded  */
  if(addr != lower_page_bound(buffer+size))
  {
    addr = (uint8_t*)lower_page_bound(buffer + size);
    p = page_find (addr,thread_current()->process->sup_table);
    if (p != NULL && !p->loaded)
      load_page (p);
  }
  
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
page_destroy (struct hash_elem* e, void* aux UNUSED)
{
  struct page* sup_page = hash_entry(e, struct page, elem);
  
  page_free(sup_page);
  
  free(sup_page); 
}

/*  Given an additional page table, "copies" the entries that map to the data + code segment 
    to the new page table */
static void
page_copy (struct hash_elem* e, void* sup_table)
{
  struct sup_table* sup = (struct sup_table*)sup_table;
  struct page* page = hash_entry(e, struct page, elem);
  printf("page->upage = %p\n",page->upage);
  
  if (page->file == thread_current()->process->process_file)
    page_table_add(page, sup);  
}

void
page_table_copy (struct sup_table* source, struct sup_table* dest)
{
  size_t i;
  struct hash* h = &source->page_table;
  
  for (i = 0; i < h->bucket_cnt; i++) 
    {
      struct list *bucket = &h->buckets[i];
      struct list_elem *elem, *next;

      for (elem = list_begin (bucket); elem != list_end (bucket); elem = next) 
        {
          next = list_next (elem);
          page_copy (list_elem_to_hash_elem (elem), (void*)dest);
        }
    }
}

/* Debug functions */

static void
print_page (struct hash_elem* e, void* aux UNUSED)
{
  struct page* page = hash_entry(e, struct page, elem);
  
  printf("Page addr = %p\t",page->upage);
  printf("Page loaded = %d\n", page->loaded);
  printf("Page writable = %d\n", page->writable);
}


void
debug_page_table (struct sup_table* sup)
{
  hash_apply (&sup->page_table,print_page);
  printf("\n");
}

void
page_free(struct page* sup_page)
{
  ASSERT(sup_page->owner == thread_current());
  
  if(sup_page->loaded)
  {
    if(sup_page->valid)
    {
      frame_free(sup_page);
    }
    else
    {
      swap_free(sup_page);
    }
  }
}
