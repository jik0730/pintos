#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include <debug.h>
#include "userprog/pagedir.h"
#include <stdio.h>
#include "vm/frame.h"
#include "threads/synch.h"
#include "vm/page.h"

/* TODO List
*/

struct fte* init_fte (const void* upage, const void* kpage);
struct fte* find_fte (const void* upage);
struct fte* find_victim_fte (void);
void remove_victim (void* upage, void* kpage, struct thread* t);
void releaes_victim (struct fte* victim, size_t idx);

struct list frame_table;
struct lock frame_lock;

/* Ingyo: Init frame.h */
void
frame_init (void)
{
  list_init (&frame_table);
  lock_init (&frame_lock);
}

/* Ingyo: Add frame table entry to frame table with upage. */
struct fte*
add_fte (const void* upage, enum palloc_flags flag)
{
  ASSERT ((int) upage % 4096 == 0);

  lock_acquire (&frame_lock);
  void* kpage = palloc_get_page (flag);

  struct fte* fte;
  if (kpage != NULL) {
    fte = init_fte (upage, kpage);
    lock_release (&frame_lock);
    return fte;
  } else {
    struct fte* victim = find_victim_fte ();
    kpage = victim->kpage;
    size_t idx = swap_out (kpage);
    release_victim (victim, idx);
    remove_victim (victim->upage, victim->kpage, victim->thread);

    ASSERT (kpage == palloc_get_page (flag));
    fte = init_fte (upage, kpage);
    lock_release (&frame_lock);
    return fte;
  }
}

void
release_victim (struct fte* victim, size_t idx)
{
  struct spte s;
  s.upage = victim->upage;
  struct hash_elem* e = hash_find (&victim->thread->spt, &s.elem);
  if (e == NULL) return ;

  struct spte* spte = hash_entry (e, struct spte, elem);
  ASSERT (spte != NULL);
  ASSERT (spte->is_loaded == true);

  pagedir_clear_page (spte->fte->thread->pagedir, spte->upage);
  spte->fte = NULL;
  spte->kpage = NULL;
  spte->is_loaded = false;
  spte->type = SWAP;
  spte->idx = idx;
}

/* Ingyo: Remove frame table entry from frame table with upage. */
void
remove_fte (const void* upage)
{
  ASSERT ((int) upage % 4096 == 0);
  lock_acquire (&frame_lock);  
  struct fte* fte = find_fte (upage);
  ASSERT (fte != NULL);

  palloc_free_page (fte->kpage);
  list_remove (&fte->elem);
  free (fte);
  lock_release (&frame_lock);
}

/* Ingyo: Find victim fte by Enhanced Second Change Algorithm. */
struct fte*
find_victim_fte (void)
{
  struct list_elem* e;   
  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage;
    if (!pagedir_is_accessed (pd, upage)
        && !pagedir_is_dirty (pd, upage))
    {
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
    void* upage = fte->upage;
    if (!pagedir_is_accessed (pd, upage)
        && pagedir_is_dirty (pd, upage))
      return fte;
  }

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage;
    if (pagedir_is_accessed (pd, upage)
        && !pagedir_is_dirty (pd, upage))
      return fte;
  }

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    uint32_t* pd = fte->thread->pagedir;
    void* upage = fte->upage;
    if (pagedir_is_accessed (pd, upage)
        && pagedir_is_dirty (pd, upage))
      return fte;
  }

  ASSERT (0);
}

struct fte*
init_fte (const void* upage, const void* kpage)
{
  ASSERT ((int) upage % 4096 == 0);

  struct fte* fte = malloc (sizeof(struct fte));
  fte->upage = upage;
  fte->kpage = kpage;
  fte->thread = thread_current ();
  list_push_back (&frame_table, &fte->elem);

  return fte;
}

/* Ingyo: Find fte by upage. */
struct fte*
find_fte (const void* upage)
{
  ASSERT ((int) upage % 4096 == 0);

  struct thread* cur = thread_current ();
  struct list_elem* e;

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    if (fte->thread == cur && fte->upage == upage)
      return fte;
  }

  return NULL;
}

void
remove_victim (void* upage, void* kpage, struct thread* t)
{
  struct list_elem* e;

  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    if (fte->thread == t && fte->upage == upage && fte->kpage == kpage) {
      palloc_free_page (fte->kpage);
      list_remove (&fte->elem);
      free (fte);
      break;
    }
  }
}

void
remove_victim_public (void* upage, void* kpage, struct thread* t)
{
  struct list_elem* e;

  lock_acquire (&frame_lock);
  for (e=list_begin (&frame_table); e!=list_end (&frame_table);
       e=list_next (e))
  {
    struct fte* fte = list_entry (e, struct fte, elem);
    if (fte->thread == t && fte->upage == upage && fte->kpage == kpage) {
      palloc_free_page (fte->kpage);
      list_remove (&fte->elem);
      free (fte);
      break;
    }
  }
  lock_release (&frame_lock);
}
