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

/* Ingyo: Init spage_table. */
void
spage_init (struct hash* h)
{
  hash_init (h, spage_hash_func, spage_less_func, NULL);
}

void
spage_destroy (struct hash* h)
{
  // TODO
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
      // TODO: Load swapped file.
      break;
    case MMFILE:
      // TODO: Load MMfile.
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

    if (file_read_at (spte->file, spte->kpage, spte->read_bytes, 
                      spte->ofs) != (int) spte->read_bytes)
    {
      remove_fte (spte->upage);
      return success;
    }
    memset (spte->kpage+spte->read_bytes, 0, PGSIZE-spte->read_bytes);

    if (!install_page (spte->upage, spte->kpage, spte->writable))
    {
      remove_fte (spte->upage);
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
  // TODO
}

/* Ingyo: Load exec_file from struct file*. */
bool
load_mmfile (struct spte* spte)
{
  // TODO
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
  spte->type = EXEC;
  spte->file = file;
  spte->read_bytes = read_bytes;
  spte->writable = writable;
  spte->is_loaded = false;
  spte->ofs = ofs;
  struct hash_elem* e = hash_insert (&thread_current ()->spt, 
                                     &spte->elem);
  ASSERT (e == NULL);
  return spte;
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

//printf ("lazy load at: %X, file_pos: %d, file: %d\n", upage, file_tell(file), file);

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
  struct fte* fte = add_fte (upage, PAL_USER | PAL_ZERO);
  if (spte == NULL || fte == NULL) return success;
  spte->upage = upage;
  spte->kpage = fte->kpage;
  spte->type = SWAP;
  spte->file = NULL;
  spte->read_bytes = -1;
  spte->writable = true;
  spte->is_loaded = true;
  spte->ofs = -1;
  struct hash_elem* e = hash_insert (&thread_current ()->spt, 
                                     &spte->elem);
  ASSERT (e == NULL);

  if (!install_page (spte->upage, spte->kpage, spte->writable))
    return success;

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
  // TODO: To be implemented.
}


