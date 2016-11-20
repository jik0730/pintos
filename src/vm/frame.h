#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include "threads/thread.h"
#include <list.h>

struct fte
  {
    void* upage_addr;		/* Ingyo: User page's virtual addr.	*/
    void* kpage_addr;		/* Ingyo: Kernel page's virtual addr, 
				   which corresponds to physical addr.	*/
    struct thread* thread;	/* Ingyo: Thread having fte. 		*/
    struct list_elem elem;	/* Ingyo: List elem for frame table.	*/
  };

void frame_init (void);
void add_fte (const void* upage_addr);
void add_fte_test (const void* upage_addr, const void* kpage_addr);

#endif /* vm/frame.h */
