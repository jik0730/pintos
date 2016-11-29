#include "vm/swap.h"

#define SWAP_FREE 0
#define SWAP_IN_USE 1

struct bitmap* swap_table;
struct block* swap_block;
struct lock swap_lock;

/* TODO List: 
*/

/* Ingyo: Init swap table. */
void
swap_init (void)
{
  swap_block = block_get_role (BLOCK_SWAP);
  swap_table = bitmap_create (block_size (swap_block) *
                              BLOCK_SECTOR_SIZE / PGSIZE);
  bitmap_set_all (swap_table, SWAP_FREE);
  lock_init (&swap_lock);
}

/* Ingyo: Swap in function. 
          Input Params: kpage, swap index for bitmap.
*/
bool
swap_in (void* kpage, size_t idx)
{
  ASSERT (bitmap_test (swap_table, idx) == SWAP_IN_USE);

  lock_acquire (&swap_lock);
  bitmap_flip (swap_table, idx);

  for (int i=0; i<PGSIZE/BLOCK_SECTOR_SIZE; i++)
  {
    block_read (swap_block, (idx*PGSIZE/BLOCK_SECTOR_SIZE)+i,
                kpage+(i*BLOCK_SECTOR_SIZE));
  }
  lock_release (&swap_lock);
  return true;
}

/* Ingyo: Swap out function. 
          Input Param: kpage.
*/
size_t
swap_out (void* kpage)
{
  lock_acquire (&swap_lock);
  size_t idx = bitmap_scan_and_flip (swap_table, 0, 1, SWAP_FREE);
  ASSERT (idx != BITMAP_ERROR);

  for (int i=0; i<PGSIZE/BLOCK_SECTOR_SIZE; i++)
  {
    block_write (swap_block, (idx*PGSIZE/BLOCK_SECTOR_SIZE)+i,
                 kpage+(i*BLOCK_SECTOR_SIZE));
  }

  lock_release (&swap_lock);
  return idx;
}



