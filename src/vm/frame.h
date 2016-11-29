#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include "threads/thread.h"
#include <list.h>
#include "threads/palloc.h"
#include "vm/swap.h"

struct fte
  {
    void* upage;		/* Ingyo: User page's virtual addr.	*/
    void* kpage;		/* Ingyo: Kernel page's virtual addr, 
				   which corresponds to physical addr.	*/
    struct thread* thread;	/* Ingyo: Thread having fte. 		*/
    struct list_elem elem;	/* Ingyo: List elem for frame table.	*/
  };

void frame_init (void);
struct fte* add_fte (const void* upage, enum palloc_flags flag);
void remove_fte (const void* upage);
void remove_victim_public (void* upage, void* kpage, struct thread* t);

#endif /* vm/frame.h */
