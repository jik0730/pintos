#include "vm/page.h"
#include <debug.h>

unsigned spage_hash_func (const struct hash_elem* e, void* aux UNUSED);
bool spage_less_func (const struct hash_elem* a,
                      const struct hash_elem* b,
                      void* aux UNUSED);
void spage_action_func (struct hash_elem* e, void* aux UNUSED);

/* Ingyo: Init spage_table. */
void
spage_init (struct hash* hash)
{
  hash_init (hash, spage_hash_func, spage_less_func, NULL);
}

/* Ingyo: Add spte to the spage_table. */
bool
add_spte (struct hash* hash)
{
  return true;
}

/* Ingyo: Remove spte from the spage_table. */
struct spte*
remove_spte (struct hash* hash)
{

}

/* Ingyo: Get spte from the spage_table with upage_addr. */
struct spte*
get_spte (struct hash* hash, void* upage_addr)
{
  
}

/* Ingyo: Init spte. */
void
init_spte ()
{

}

/* Ingyo: Functions needed for hash table. */
unsigned spage_hash_func (const struct hash_elem* e, void* aux UNUSED)
{
  struct spte* spte = hash_entry (e, struct spte, elem);
  return hash_int ((int) spte->upage_addr);
}

bool spage_less_func (const struct hash_elem* a,
                      const struct hash_elem* b,
                      void* aux UNUSED)
{
  struct spte* s_a = hash_entry (a, struct spte, elem);
  struct spte* s_b = hash_entry (b, struct spte, elem);
  if (s_a->upage_addr < s_b->upage_addr) return true;
  return false;
}

void spage_action_func (struct hash_elem* e, void* aux UNUSED)
{
  // TODO: To be implemented.
}


