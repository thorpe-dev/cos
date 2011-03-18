#include "vm/page.h"

#define NOT_YET_SWAPPED 0xFFFFFFFF

void swap_init(void);
void swap_out(struct page* sup_page);
void swap_in(struct page* sup_page);
void swap_free(struct page* sup_page);