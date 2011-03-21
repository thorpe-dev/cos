#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/process.h"
#include <hash.h>
#include <string.h>
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"



//TODO: Remove
#include <stdio.h>
#include "userprog/pagedir.h"

//TODO: Finish synchronisation

static bool page_less (const struct hash_elem* p1, const struct hash_elem* p2, void* aux);
static unsigned page_hash (const struct hash_elem* elem, void* aux);
static void page_destroy (struct hash_elem* e, void* aux);
static void page_copy (struct hash_elem* e, void* sup_table);
static void print_page (struct hash_elem* e, void* aux UNUSED);

/* Mutually exclusive access to frame/swap management 
   and supplementary page table access */
static struct lock vm_lock;


bool
page_table_init (struct sup_table* sup) 
{
  lock_init(&vm_lock);
  return hash_init(&sup->page_table, page_hash, page_less, NULL);
}


bool
page_table_add (struct page* p, struct sup_table* table)
{
  return(hash_insert (&table->page_table, &p->elem) == NULL);
}


bool
page_table_remove (struct page* p, struct sup_table* table)
{
  if (hash_delete (&table->page_table, &p->elem) != NULL)
  {
    page_free(p);
    free(p);
    return true;
  }
  else
    return false;
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
page_find (uint8_t* upage, struct sup_table* sup)
{
  struct page page;
  struct hash_elem* value;
  
  page.upage = upage;
  
  value = hash_find(&sup->page_table, &page.elem);
  if (value == 0)
    return NULL;

  return hash_entry(value, struct page, elem);
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
  struct sup_table* sup;
  uint8_t* round_buffer = lower_page_bound(buffer);
  sup = thread_current()->process->sup_table;
     
  for(i = 0; i <= size / PGSIZE ; i++)
  {
    addr = round_buffer + (i*PGSIZE);
    p = page_find (addr,sup);
    if (p != NULL) {
      if (!p->valid)
        load_page (p);
    }
    else
      p = page_create (addr, true);
  }
  
  /* If buffer is right at a page boundary, might miss a page to be loaded - check that it is loaded  */
  if(addr != lower_page_bound(buffer+size))
  {
    addr = (uint8_t*)lower_page_bound(buffer + size);
    p = page_find (addr,sup);
    if (p != NULL) {
      if (!p->valid)
        load_page (p);
    }
    else
      p = page_create (addr,true);
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
  struct thread* t = thread_current();
  struct page* sup_page = hash_entry(e, struct page, elem);
  /* If the current process is not the page owner, 
     means the page is still being shared, so just remove
     the entry from our own page directory */
  
  if (sup_page->owner != t) {
    pagedir_clear_page(t->pagedir, sup_page->upage);
  }
  else
  {
    page_free(sup_page);
    free(sup_page);
  }
}

/*  Given an additional page table, "copies" the entries that map to the data + code segment 
    to the new page table */
static void
page_copy (struct hash_elem* e, void* sup_table)
{
  struct sup_table* sup = (struct sup_table*)sup_table;
  struct page* page = hash_entry(e, struct page, elem);
  //printf("page->upage = %p\n",page->upage);
  
  if ((page != NULL) 
      && (page->file == thread_current()->process->process_file)) 
      //&& !page->writable)
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
  
  printf("Upage = %p\t",page->upage);
  printf("valid = %d\t", page->valid);
  printf("loaded = %d\t", page->loaded);
  printf("writable = %d\t", page->writable);
  printf("struct addr = %p\n", page);
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

  if(sup_page->valid)
  {
    frame_free(sup_page);
  }
  else
  {
    swap_free(sup_page);
  }
}


/* Loads a single page starting at offset OFS in FILE at address
UPAGE. A page of virtual memory is initialized, as follows:

- READ_BYTES bytes at UPAGE must be read from FILE
starting at offset OFS.

- ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

The page initialized by this function must be writable by the
user process if WRITABLE is true, read-only otherwise.

Return true if successful, false if a memory allocation error
or disk read error occurs. */
void
load_page (struct page* p)
{
  struct file* file;
  struct thread* t;
  bool old_writable;
  
  ASSERT ((p->read_bytes + p->zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (p->upage) == 0);
  ASSERT (p->ofs % PGSIZE == 0);
  
  file = p->file;
  
  t = thread_current();
  
  /* Temporarily set writable to true (so we can actually load the page) */
  old_writable = p->writable;
  if(!old_writable)
    p->writable = true;
  
  /* Get a page of memory. */
  //lock_acquire(&vm_lock);
  void* kpage = frame_get(PAL_USER, p);
  //lock_release(&vm_lock);
  
  /* Load this page. */
  if (file != NULL)
  {
    lock_acquire(&filesys_lock);
    if (file_read_at(file, p->upage, p->read_bytes, p->ofs) != (int) p->read_bytes)
    {
      page_free(p);
      lock_release(&filesys_lock);
      PANIC("Load page failed - file could not be found");
    }
    lock_release(&filesys_lock);
  }
  
  memset (p->upage + p->read_bytes, 0, p->zero_bytes);
  
  /* Restore old "false" writable flag if necessary */
  if(!old_writable)
  {
    p->writable = false;
    pagedir_clear_page(t->pagedir, p->upage);
    pagedir_set_page(t->pagedir, p->upage, kpage, false);
  }

  p->loaded = true;
  
  /* Clear dirty bit */
  pagedir_set_accessed(t->pagedir, p->upage, false);
}


/* Creates a supplemental page and adds it to the sup page table, but 
   does not allocate any physical memory */
struct page*
page_create (uint8_t* upage, bool writable)
{
  struct page* sup_page;
  
  sup_page = malloc(sizeof(struct page));
  
  sup_page->upage = upage;
  sup_page->writable = writable;
  sup_page->owner = thread_current();
  sup_page->swap_idx = NOT_YET_SWAPPED;
  sup_page->loaded = false;
  sup_page->valid = false;
  sup_page->read_bytes = 0;
  sup_page->zero_bytes = 0;
  sup_page->ofs = 0;
  
  page_table_add(sup_page, thread_current()->process->sup_table);  
  
  return sup_page;
}


struct page*
page_allocate(void* upage, enum palloc_flags flags, bool writable)
{
  struct page* sup_page;
  
  lock_acquire(&vm_lock);

  sup_page = page_create(upage, writable);
  
  /* frame_get() installs the page into the page directory
      and sets the valid flag for us */
  frame_get(PAL_USER | flags, sup_page);
  
  /* Add to supplementary page table */
  page_table_add(sup_page, thread_current()->process->sup_table);
  
  lock_release(&vm_lock);
    
  return sup_page;
}

void
page_swap_in(struct page* sup_page)
{
  lock_acquire(&vm_lock);
  swap_in(sup_page);
  lock_release(&vm_lock);
}
