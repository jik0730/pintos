#include <stdio.h>
#include <stdbool.h>
#include <hash.h>
#include "filesys/file.h"

enum spte_type
{
  EXEC,
  SWAP,
  MMFILE
};

struct spte
{
  void* upage;
  void* kpage;
  bool is_loaded;
  enum spte_type type;
  struct hash_elem elem;

  // For exec file
  struct file* file;
  off_t ofs;
  uint32_t read_bytes;
  bool writable;
};

void spage_init (struct hash* h);
bool load_page (struct spte* spte);
struct spte* get_spte (void* upage);
bool lazy_load_segment (struct file* file, off_t ofs, uint8_t* upage,
                        uint32_t read_bytes, uint32_t zero_bytes, 
                        bool writable);
bool stack_growth (void* upage);
