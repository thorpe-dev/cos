#include "vm/page.h"

void swap_init(void);
void* swap_out(struct page* page);
uint8_t* swap_in(struct page* page);
