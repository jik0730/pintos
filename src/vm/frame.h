#include <stdbool.h>
#include "thread/thread.h"
#include <list.h>

struct fte
  {
    void* upage_addr;		/* Ingyo: User page's virtual addr.	*/
    void* kpage_addr;		/* Ingyo: Kernel page's virtual addr, 
				   which corresponds to physical addr.	*/
    struct thread* thread;	/* Ingyo: Thread having fte. 		*/
    struct list_elem elem;	/* Ingyo: List elem for frame table.	*/
  };
