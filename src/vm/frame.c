#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include <debug.h>
#include "userprog/pagedir.h"
#include <stdio.h>

/* TODO List
   1. Deallocating frame table entries. (where?)
   2. Adding functionality of swapping.
   3. Page aligned????? -> ASSERT...
*/

struct fte* init_fte (const void* upage_addr, const void* kpage_addr);
struct fte* find_fte (const void* upage_addr);
void remove_fte (const void* upage_addr);
struct fte* find_victim_fte (void);

// Ingyo: Globally declared frame table.
struct list frame_table;

// Ingyo: Init frame.h
void
frame_init (void)
{
  list_init (&frame_table);
}

// Ingyo: Add frame table entry to frame table with upage_addr.
void
add_fte (const void* upage_addr)
{
  ASSERT (upage_addr % 4096 == 0);

  void* kpage_addr = palloc_get_page (PAL_USER);
  // TODO: Implement swapping, otherwise kernel panic.
  ASSERT (kpage_addr != NULL);

  struct fte* fte;
  if (kpage_addr != NULL) {
    fte = init_fte (upage_addr, kpage_addr);
  } else {
    // TODO: Swapping and find victim fte.
  }
}

// Ingyo: Overloaded add_fte for testing.
void
add_fte_test (const void* upage_addr, const void* kpage_addr)
{
  ASSERT (upage_addr % 4096 == 0);

  struct fte* fte = init_fte (upage_addr, kpage_addr);
}

// Ingyo: Remove frame table entry from frame table with upage_addr.
void
remove_fte (const void* upage_addr)
{
  ASSERT (upage_addr % 4096 == 0);

  struct fte* fte = find_fte (upage_addr);
  ASSERT (fte != NULL);

  palloc_free_page (fte->kpage_addr);
  // Clear pd entry. TODO: Do we really need this here??
  pagedir_clear_page (fte->thread->pagedir, upage_addr);

  list_remove (&fte->elem);
  free (fte);
}

// Ingyo: Find victim fte by Enhanced Second Change Algorithm.
struct fte*
find_victim_fte (void)
{
  struct list_elem* e;   
  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage_addr;
    if (!pagedir_is_accessed (pd, upage)
        && !pagedir_is_dirty (pd, upage))
    {
      printf ("Is it possible????\n");
      return fte;
    }
    if (pagedir_is_accessed (pd, upage))
      pagedir_set_accessed (pd, upage, false);
   
  }

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage_addr;
    if (!pagedir_is_accessed (pd, upage)
        && pagedir_is_dirty (pd, upage))
      return fte;
  }

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage_addr;
    if (pagedir_is_accessed (pd, upage)
        && !pagedir_is_dirty (pd, upage))
      return fte;
  }

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage_addr;
    if (pagedir_is_accessed (pd, upage)
        && pagedir_is_dirty (pd, upage))
      return fte;
  }

  ASSERT (0);
}

struct fte*
init_fte (const void* upage_addr, const void* kpage_addr)
{
  ASSERT (upage_addr % 4096 == 0);

  struct fte* fte = malloc (sizeof(struct fte));
  fte->upage_addr = upage_addr;
  fte->kpage_addr = kpage_addr;
  fte->thread = thread_current ();
  // TODO: Do we need ordered list by somewhat??
  list_push_back (&frame_table, &fte->elem);

  return fte;
}

// Ingyo: Find fte by upage_addr.
struct fte*
find_fte (const void* upage_addr)
{
  ASSERT (upage_addr % 4096 == 0);

  struct thread* cur = thread_current ();
  struct list_elem* e;

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    if (fte->thread == cur && fte->upage_addr == upage_addr)
      return fte;
  }

  return NULL;
}


