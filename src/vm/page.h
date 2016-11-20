#include <stdio.h>
#include <stdbool.h>
#include <hash.h>

enum spte_type
{
  FILE,
  SWAP,
  MMFILE
};

struct spte
{
  void* upage_addr;
  void* kpage_addr;
  bool is_loaded;
  enum spte_type type;
  struct hash_elem elem;
};
