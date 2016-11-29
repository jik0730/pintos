#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <stdbool.h>
#include <bitmap.h>
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

void swap_init (void);
bool swap_in (void* kpage, size_t idx);
size_t swap_out (void* kpage);

#endif /* vm/swap.h */
