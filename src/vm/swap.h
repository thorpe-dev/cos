#include "vm/page.h"

#define NOT_YET_SWAPPED 0xFFFFFFFF

void swap_init(void);
void swap_out(struct page* page);
void* swap_in(struct page* page);
