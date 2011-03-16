#include "vm/page.h"

void swap_init(void);
void* swap_out(struct page* page);
void swap_in(struct page* page);
