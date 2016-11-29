#include "vm/page.h"
#include <debug.h>
#include "userprog/process.h"
#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

unsigned spage_hash_func (const struct hash_elem* e, void* aux UNUSED);
bool spage_less_func (const struct hash_elem* a,
                      const struct hash_elem* b,
                      void* aux UNUSED);
void spage_action_func (struct hash_elem* e, void* aux UNUSED);
bool load_exec (struct spte* spte);
struct spte* init_exec_spte (struct file* file, off_t ofs, 
                             uint8_t* upage, uint32_t read_bytes, 
                             bool writable);
struct spte* init_mmfile_spte (struct file* file, off_t ofs, 
                               uint8_t* upage, uint32_t read_bytes, 
                               bool writable);
bool load_swap (struct spte* spte);
bool load_mmfile (struct spte* spte);

/* Ingyo: Init spage_table. */
void
spage_init (struct hash* h)
{
  hash_init (h, spage_hash_func, spage_less_func, NULL);
}

/* Ingyo: Destroy spage_table. */
void
spage_destroy (struct hash* h)
{
  hash_destroy (h, spage_action_func);
}

/* Ingyo: Load page with appropriate spte. */
bool
load_page (struct spte* spte)
{
  int success = false;
  
  switch (spte->type)
  {
    case EXEC:
      success = load_exec (spte);
      break;
    case SWAP:
      success = load_swap (spte);
      break;
    case MMFILE:
      success = load_mmfile (spte);
      break;
    default:
      return success;
  }

  if (!success) return false;
  spte->is_loaded = true;
  return success;
}

/* Ingyo: Load exec_file from struct file*. */
bool
load_exec (struct spte* spte)
{
  ASSERT (spte != NULL);
  ASSERT (spte->upage != NULL);

  bool success = false;
  struct fte* fte = add_fte (spte->upage, PAL_USER);
  if (fte == NULL)
  {
    return success;
  }
  else
  {
    spte->kpage = fte->kpage;
    spte->fte = fte;
    if (file_read_at (spte->file, spte->kpage, spte->read_bytes, 
                      spte->ofs) != (int) spte->read_bytes)
    {
      remove_fte (spte->upage);
      spte->kpage = NULL;
      spte->fte = NULL;
      return success;
    }
    memset (spte->kpage+spte->read_bytes, 0, PGSIZE-spte->read_bytes);
    if (!install_page (spte->upage, spte->kpage, spte->writable))
    {
//      remove_fte (spte->upage);
      palloc_free_page (spte->fte->kpage);
      list_remove (&spte->fte->elem);
      free (spte->fte);
      spte->kpage = NULL;
      spte->fte = NULL;
      hash_delete (&thread_current ()->spt, &spte->elem);
      free (spte);
      return success;
    }
    success = true;
    return success;
  }
}

/* Ingyo: Load exec_file from struct file*. */
bool
load_swap (struct spte* spte)
{
  ASSERT (spte->is_loaded == false);
  ASSERT (spte->idx != -1);
  ASSERT (spte->kpage == NULL);

  bool success = false;
  struct fte* fte = add_fte (spte->upage, PAL_USER);
  ASSERT (fte != NULL);
  spte->kpage = fte->kpage;
  spte->fte = fte;

  if (!install_page (spte->upage, spte->kpage, spte->writable))
  {
    remove_fte (spte->upage);
//    free (spte->fte);
    spte->kpage = NULL;
    spte->fte = NULL;
    return success;
  }

  success = swap_in (spte->kpage, spte->idx);
  return success;
}

/* Ingyo: Load exec_file from struct file*. */
bool
load_mmfile (struct spte* spte)
{
  ASSERT (spte != NULL);
  ASSERT (spte->upage != NULL);

  bool success = false;
  struct fte* fte = add_fte (spte->upage, PAL_USER);
  if (fte == NULL)
  {
    return success;
  }
  else
  {
    spte->kpage = fte->kpage;
    spte->fte = fte;
    if (file_read_at (spte->file, spte->kpage, spte->read_bytes, 
                      spte->ofs) != (int) spte->read_bytes)
    {
      remove_fte (spte->upage);
      spte->kpage = NULL;
      spte->fte = NULL;
      return success;
    }
    memset (spte->kpage+spte->read_bytes, 0, PGSIZE-spte->read_bytes);
    if (!install_page (spte->upage, spte->kpage, spte->writable))
    {
      remove_fte (spte->upage);
//      free (spte->fte);
      spte->kpage = NULL;
      spte->fte = NULL;
      return success;
    }
    success = true;
    return success;
  }
}

/* Ingyo: Unmap sptes from MUNMAP. */
bool
munmap_sptes (struct mmap_file* mf)
{
  bool success = false;

  off_t ofs = 0;
  struct file* f = mf->file;
  off_t fl = file_length (f);
  void* upage = mf->upage;

  while (ofs < fl)
  {
    struct spte* spte = get_spte (upage);
    ASSERT (spte != NULL);

    if (pagedir_is_dirty (thread_current ()->pagedir, spte->upage))
    {
      off_t written = file_write_at (f, upage, PGSIZE, ofs);
      ASSERT (written != 0);
    }

    if (spte->is_loaded)
    {
      release_spte (spte->upage, spte->idx);
      remove_fte (spte->upage);
    }

    hash_delete (&thread_current ()->spt, &spte->elem);
    free (spte);
    ofs += PGSIZE;
    upage += PGSIZE;
  }
  file_close (f);
  success = true;
  return success;
}

/* Ingyo: Get spte from the spage_table with upage. 
          Return NULL if cannot find.
*/
struct spte*
get_spte (void* upage)
{
  struct spte s;
  s.upage = upage;
  struct hash_elem* e = hash_find (&thread_current ()->spt, &s.elem);
  if (e == NULL) return NULL;
  return hash_entry (e, struct spte, elem);
}

/* Ingyo: Create initialized empty spte. */
struct spte*
init_exec_spte (struct file* file, off_t ofs, uint8_t* upage,
                uint32_t read_bytes, bool writable)
{
  struct spte* spte = malloc (sizeof (struct spte));
  if (spte == NULL) return NULL;
  spte->upage = (void*) upage;
  spte->kpage = NULL;
  spte->fte = NULL;
  spte->type = EXEC;
  spte->file = file;
  spte->read_bytes = read_bytes;
  spte->writable = writable;
  spte->is_loaded = false;
  spte->ofs = ofs;
  spte->idx = -1;
  struct hash_elem* e = hash_insert (&thread_current ()->spt, 
                                     &spte->elem);
  ASSERT (e == NULL);
  return spte;
}

/* Ingyo: Create initialized empty spte for mmfile. */
struct spte*
init_mmfile_spte (struct file* file, off_t ofs, uint8_t* upage,
                uint32_t read_bytes, bool writable)
{
  struct spte* spte = malloc (sizeof (struct spte));
  if (spte == NULL) return NULL;
  spte->upage = (void*) upage;
  spte->kpage = NULL;
  spte->fte = NULL;
  spte->type = MMFILE;
  spte->file = file;
  spte->read_bytes = read_bytes;
  spte->writable = writable;
  spte->is_loaded = false;
  spte->ofs = ofs;
  spte->idx = -1;
  struct hash_elem* e = hash_insert (&thread_current ()->spt, 
                                     &spte->elem);
  ASSERT (e == NULL);
  return spte;
}

/* Ingyo: Release spte meaning that deallocating from physical memory. */
void
release_spte (void* upage, size_t idx)
{
  struct spte* spte = get_spte (upage);
  ASSERT (spte != NULL);
  ASSERT (spte->is_loaded == true);

  pagedir_clear_page (spte->fte->thread->pagedir, spte->upage);
  spte->fte = NULL;
  spte->kpage = NULL;
  spte->is_loaded = false;
  spte->type = SWAP;
  if (spte->type == SWAP)
  {
    spte->idx = idx;
  }
}

/* Ingyo: Lazy load segment. */
bool
lazy_load_segment (struct file* file, off_t ofs, uint8_t *upage,
                   uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes+zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);
  ASSERT (!get_spte (upage));

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
  {
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct spte* spte = init_exec_spte (file, ofs, upage, 
                        page_read_bytes, writable);
    if (spte == NULL) return false;

    ofs += page_read_bytes;
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    upage += PGSIZE;
  }
  return true;
}

/* Ingyo: Lazy load segment for mmfile. */
bool
lazy_load_segment_mmfile (struct file* file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes+zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
  {
    if (get_spte (upage) != NULL) return false;
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct spte* spte = init_mmfile_spte (file, ofs, upage, 
                        page_read_bytes, writable);
    if (spte == NULL) return false;

    ofs += page_read_bytes;
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    upage += PGSIZE;
  }
  return true;
}

/* Ingyo: Stack Growth. */
bool
stack_growth (void* upage)
{
  bool success = false;
  struct spte* spte = malloc (sizeof (struct spte));
  if (spte == NULL) return success;
  struct fte* fte = add_fte (upage, PAL_USER | PAL_ZERO);
  if (fte == NULL) return success;
  spte->upage = upage;
  spte->kpage = fte->kpage;
  spte->fte = fte;
  spte->type = SWAP;
  spte->file = NULL;
  spte->read_bytes = -1;
  spte->writable = true;
  spte->is_loaded = true;
  spte->ofs = -1;
  spte->idx = -1;
  struct hash_elem* e = hash_insert (&thread_current ()->spt, 
                                     &spte->elem);
  ASSERT (e == NULL);

  if (!install_page (spte->upage, spte->kpage, spte->writable))
  {
    remove_fte (spte->upage);
    hash_delete (&thread_current ()->spt, e);
    free (spte);
    return success;
  }

  success = true;
  return success; 
}

/* Ingyo: Functions needed for hash table. */
unsigned spage_hash_func (const struct hash_elem* e, void* aux UNUSED)
{
  struct spte* spte = hash_entry (e, struct spte, elem);
  return hash_int ((int) spte->upage);
}

bool spage_less_func (const struct hash_elem* a,
                      const struct hash_elem* b,
                      void* aux UNUSED)
{
  struct spte* s_a = hash_entry (a, struct spte, elem);
  struct spte* s_b = hash_entry (b, struct spte, elem);
  if (s_a->upage < s_b->upage) return true;
  return false;
}

void spage_action_func (struct hash_elem* e, void* aux UNUSED)
{
  struct spte* spte = hash_entry (e, struct spte, elem);

  if (spte->is_loaded) {
    pagedir_clear_page (spte->fte->thread->pagedir, spte->upage);
    remove_victim_public (spte->upage, spte->kpage, spte->fte->thread);
  }

  if (spte->file != NULL) {
//    file_close (spte->file);
  }
  list_remove (&spte->elem);
  free (spte);
}


