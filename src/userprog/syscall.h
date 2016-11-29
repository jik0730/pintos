#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>
#include "filesys/file.h"

typedef int mapid_t;

struct mmap_file
{
  mapid_t mid;
  void* upage;
  struct file* file;
  struct list_elem elem;
};

void syscall_init (void);
void exit (int status);
void mmap_file_list_destroy (struct list* mfl);
struct mmap_file* find_mmap_file (mapid_t mapid);

#endif /* userprog/syscall.h */
