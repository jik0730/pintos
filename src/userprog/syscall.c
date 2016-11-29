#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include <string.h>
#include "vm/page.h"

#define ARG_MAX 3 // Ingyo: define ARG_MAX.
#define EXIT_SUCCESS 0 // Ingyo: define exit s/f.
#define EXIT_FAILURE -1
#define USER_VADDR_BOTTOM 0x08048000
typedef int pid_t; // Ingyo: define pid_t(Process indentifier).

/* Ingyo: Personally defined functions. */
void get_arguments (struct intr_frame* _f, 
                    int* _args, int num_args);
void check_ptr_valid (void* esp);
struct process_file* find_file_by_fd (int fd);
struct mmap_file* find_mmap_file (mapid_t mapid);

static void syscall_handler (struct intr_frame *);
void halt (void);
void exit (int status);
pid_t exec (const char* cme_line);
int wait (pid_t pid);
int read (int fd, void* buffer, unsigned size);
int write (int fd, const void* buffer, unsigned size);
bool create (const char* file, unsigned initial_size);
int open (const char* file);
int filesize (int fd);
void close (int fd);
unsigned tell (int fd);
void seek (int fd, unsigned position);
bool remove (const char* file);
mapid_t mmap (int fd, void* addr);
void munmap (mapid_t mapid);

void
syscall_init (void) 
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /* Ingyo: Declare some variables for syscall_handler. */
  int* args[ARG_MAX];
  check_ptr_valid (f->esp);

  struct thread* cur = thread_current ();

  /* Ingyo: Modify to accept system call with sys_num. */
  switch (*(int*)(f->esp))
  {
    case SYS_HALT:
    {
      halt ();
      break;
    }
    case SYS_EXIT:
    {
      get_arguments (f, args, 1);
      exit (*(int*)args[0]);
      break;
    }
    case SYS_EXEC:
    {
      get_arguments (f, args, 1);
      f->eax = exec ((const char*)*(int*)args[0]);
      break;
    }
    case SYS_WAIT:
    {
      get_arguments (f, args, 1);
      f->eax = wait (*(int*)args[0]);
      break;
    }
    case SYS_WRITE:
    {
      get_arguments (f, args, 3);
      f->eax = write (*(int*)args[0], 
                      (const void*)*(int*)args[1],
                      (unsigned)*(int*)args[2]);
      break;
    }
    case SYS_READ:
    {
      get_arguments (f, args, 3);
      f->eax = read (*(int*)args[0], (void*)*(int*)args[1],
                     (unsigned)*(int*)args[2]);
      break;
    }
    case SYS_CREATE:
    {
      get_arguments (f, args, 2);
      f->eax = create ((const char*)*(int*)args[0], 
                       (unsigned)*(int*)args[1]);
      break;
    }
    case SYS_OPEN:
    {
      get_arguments (f, args, 1);
      f->eax = open ((const char*)*(int*)args[0]);
      break;
    }
    case SYS_FILESIZE:
    {
      get_arguments (f, args, 1);
      f->eax = filesize (*(int*)args[0]);
      break;
    }
    case SYS_CLOSE:
    {
      get_arguments (f, args, 1);
      close (*(int*)args[0]);
      break;
    }
    case SYS_TELL:
    {
      get_arguments (f, args, 1);
      f->eax = tell (*(int*)args[0]);
      break;
    }
    case SYS_SEEK:
    {
      get_arguments (f, args, 2);
      seek (*(int*)args[0], (unsigned)*(int*)args[1]);
      break;
    }
    case SYS_REMOVE:
    {
      get_arguments (f, args, 1);
      f->eax = remove ((const char*)*(int*)args[0]);
      break;
    }
    case SYS_MMAP:
    {
      get_arguments (f, args, 2);
      f->eax = mmap (*(int*)args[0], (void*)*(int*)args[1]);
      break;
    }
    case SYS_MUNMAP:
    {
      get_arguments (f, args, 1);
      munmap ((mapid_t)*(int*)args[0]);
      break;
    }
    default:
    {
//      printf ("Strange syscall!!!!");
      break;
    }
  }

}

// Ingyo: Call the function power_off()
void
halt (void)
{
  shutdown_power_off ();
}

// Ingyo: exec
pid_t
exec (const char* cmd_line)
{
  check_ptr_valid (cmd_line);
  char* cpy = (char*) malloc ((strlen(cmd_line)+1)*sizeof(char));
  strlcpy (cpy, cmd_line, strlen(cmd_line)+1);
  char* token, save_ptr;
  token = strtok_r (cpy, " ", &save_ptr);
  lock_acquire (&filesys_lock);
  struct file* f = filesys_open (token);
  if (f == NULL) {
    lock_release (&filesys_lock);
    free (cpy);
    return -1;
  }
  file_close (f);
  lock_release (&filesys_lock);

  int pid = process_execute (cmd_line);
  return pid;
}

// Ingyo: exit
void
exit (int status)
{
  struct thread* cur = thread_current ();
  printf ("%s: exit(%d)\n", cur->name, status);
  // Ingyo: exit status change.
  cur->my_process->exit_status = status;
  cur->my_process->is_exit = 1;

  thread_exit ();
}

// Ingyo: wait, mainly implemented in process.c.
int
wait (pid_t pid)
{
  return process_wait (pid);
}

// Ingyo: If fd = STDOUT_FILENO, write to the console.
int
write (int fd, const void* buffer, unsigned size)
{
//  if (get_spte (pg_round_down (buffer))->type == EXEC) return -1;
//  printf ("buffer: %X\n", buffer);
  check_ptr_valid (buffer);
  if (fd == STDOUT_FILENO)
  {
    putbuf (buffer, size);
    return size;
  } else {
    lock_acquire (&filesys_lock);
    // Ingyo: Find file with specified fd in current thread.
    struct process_file* pf = find_file_by_fd (fd);
    int count = file_write (pf->file, buffer, size);
    lock_release (&filesys_lock);
    return count;
  }
}

// Ingyo: If fd = STDIN_FILENO, read from the console.
int
read (int fd, void* buffer, unsigned size)
{
//  check_ptr_valid (buffer);
// TODO: pt-grow-stk-sc test...
//printf ("bu: %X\n, fd: %d\n", buffer, fd);
  if (buffer >= PHYS_BASE || buffer<=USER_VADDR_BOTTOM)
  {
    exit (EXIT_FAILURE);
  }

  if (fd == STDIN_FILENO)
  {
    uint8_t* temp_buffer = (uint8_t*) buffer;
    for (int i=0; i<size; i++)
    {
      temp_buffer[i] = input_getc ();
    }
    return size;
  } else {
    lock_acquire (&filesys_lock);
    // Ingyo: Find file with specified fd in current thread.
    struct process_file* pf = find_file_by_fd (fd);
    if (pf == NULL) {
      lock_release (&filesys_lock);
      return -1;
    }
    int toReturn = file_read (pf->file, buffer, size);
    lock_release (&filesys_lock);
    return toReturn;
  }
}

// Ingyo: create a file.
bool
create (const char* file, unsigned initial_size)
{
  // Ingyo: NULL pointer check.
  if (file == NULL)
    exit (EXIT_FAILURE);

  // Ingyo: check file pointer whether valid.
  check_ptr_valid (file);
  lock_acquire (&filesys_lock);
  bool success = filesys_create (file, initial_size);
  lock_release (&filesys_lock);

  return success;
}

// Ingyo: open a file. return fd.
int
open (const char* file)
{
  // Ingyo: NULL pinter check, valid check.
  if (file == NULL)
    exit (EXIT_FAILURE);
  check_ptr_valid (file);

  lock_acquire (&filesys_lock);
  // Ingyo: return null if fails to open, NULL check.
  struct file* file_ = filesys_open (file);
  if (file_ == NULL || file_ == "") {
    lock_release (&filesys_lock);
    return EXIT_FAILURE;
  }

  // TODO Ingyo: if file name is same as currently running process's name, deny write.
  if (strcmp(thread_current ()->name, file) == 0) file_deny_write (file_);
  int toReturn = process_file_init (file_);
  lock_release (&filesys_lock);

  // Ingyo: find file from current thread or process.
  return toReturn;
}

// Ingyo: get filesize.
int
filesize (int fd)
{
  lock_acquire (&filesys_lock);
  // Ingyo: find file with fd in current thread or process.
  struct process_file* pf = find_file_by_fd (fd);
  if (pf == NULL) {
    lock_release (&filesys_lock);
    printf ("Something went wrong in filesize.\n");
    return -1;
  }

  // Ingyo: Use file_length() function.
  int toReturn = file_length (pf->file);
  lock_release (&filesys_lock);
  return toReturn;
}

// Ingyo: close file with fd.
void
close (int fd)
{
  lock_acquire (&filesys_lock);

  struct process_file* pf = find_file_by_fd (fd);
  if (pf == NULL) {
    lock_release (&filesys_lock);
    return ;//printf ("Something went wrong in close.\n");
  }

  file_close (pf->file);
  process_file_remove (pf);
  lock_release (&filesys_lock);
}

// Ingyo: tell next position.
unsigned
tell (int fd)
{
  lock_acquire (&filesys_lock);
  // Ingyo: find file with fd in current thread or process.
  struct process_file* pf = find_file_by_fd (fd);
  if (pf == NULL) {
    lock_release (&filesys_lock);
    return -1; //printf ("Something went wrong in tell.\n");
  }

  unsigned toReturn = file_tell (pf->file);
  lock_release (&filesys_lock);
  return toReturn;
}

// Ingyo: change a position of file to be read or written.
void
seek (int fd, unsigned position)
{
  lock_acquire (&filesys_lock);
  // Ingyo: find file with fd in current thread or process.
  struct process_file* pf = find_file_by_fd (fd);
  if (pf == NULL) {
    lock_release (&filesys_lock);
    return; //printf ("Something went wrong in seek.\n");
  }

  file_seek (pf->file, position);
  lock_release (&filesys_lock);
}

// Ingyo: Implement remove.
bool
remove (const char* file)
{
  lock_acquire (&filesys_lock);
  bool success = filesys_remove (file);
  lock_release (&filesys_lock);
  return success;
}

/* Ingyo: Implement mmap. */
mapid_t
mmap (int fd, void* addr)
{
  lock_acquire (&filesys_lock);
  struct process_file* pf = find_file_by_fd (fd);

  if (fd == 0 || fd == 1 || file_length (pf->file) == 0 || (int) addr == 0 ||
      (int) addr % 4096 != 0) {
    lock_release (&filesys_lock);
    return -1;
  }
  ASSERT (pf != NULL);

  struct file* f = file_reopen (pf->file);
  uint32_t read_bytes = file_length (f);
  uint32_t zero_bytes = (read_bytes % 4096 == 0) ? 0 :
                        ((((int) read_bytes/4096) + 1)*4096) - read_bytes;
  if (!lazy_load_segment_mmfile (f, 0, addr, read_bytes, zero_bytes, true)) {
    lock_release (&filesys_lock);
    return -1;
  }

  struct thread* cur = thread_current ();
  struct mmap_file* mf = malloc (sizeof (struct mmap_file));
  mf->mid = cur->mid++;
  mf->upage = addr;
  mf->file = f;
  list_push_back (&cur->mmap_file_list, &mf->elem);
  lock_release (&filesys_lock);
  return mf->mid;
}

/* Ingyo: Implement munmap. */
void
munmap (mapid_t mapid)
{
  struct mmap_file* mf = find_mmap_file (mapid);
  ASSERT (mf != NULL);
  if (!munmap_sptes (mf))
    ASSERT (0);
  list_remove (&mf->elem);
  free (mf);
}

/* Ingyo: find mmap_file from current thread. */
struct mmap_file*
find_mmap_file (mapid_t mapid)
{
  struct thread* cur = thread_current ();
  struct list* mmap_file_list = &(cur->mmap_file_list);
  struct list_elem* e;
  struct mmap_file* mf;

  for (e = list_begin (mmap_file_list); e != list_end (mmap_file_list);
       e = list_next (e))
  {
    mf = list_entry (e, struct mmap_file, elem);

    if (mf->mid == mapid)
      return mf;
  }
  return NULL; 
}

/* Ingyo: destroy all entry of mmap_file_list. */
void
mmap_file_list_destroy (struct list* mfl)
{
  struct list_elem* e;
  struct mmap_file* mf;

  for (e = list_begin (mfl); e != list_end (mfl);
       e = list_next (e))
  {
    mf = list_entry (e, struct mmap_file, elem);
    munmap (mf->mid);
  }

  ASSERT (list_empty (mfl));
}

/* Ingyo: Retrieve arguments from syscalls.
          Store address of args into _args. */
void
get_arguments (struct intr_frame* _f, int* _args, int num_args)
{
  int* ptr = _f->esp;
  int* args = _args;

  for (int i=0; i<num_args; i++)
  {
    ptr += 1;
    // Ingyo: pointers to arguments should not be above PHYS_BASE.
    check_ptr_valid (ptr);
    args[i] = ptr;
  }
}

/* Ingyo: Check certain pointer valid.
          If a pointer unvalid, exit(EXIT_FAILURE). */
void
check_ptr_valid (void* esp)
{
  struct thread* cur = thread_current ();
  if (esp >= PHYS_BASE || esp<=USER_VADDR_BOTTOM
      || pagedir_get_page (cur->pagedir, esp) == NULL)
  {
    exit (EXIT_FAILURE);
  }
}
 
/* Ingyo: Find a file matching with fd.
          If there is no file, return NULL. */

struct process_file*
find_file_by_fd (int fd)
{
  // Ingyo: Find file with specified fd in current thread.
  struct thread* cur = thread_current ();
  struct list* file_list = &(cur->my_process->files);
  struct list_elem* e;
  struct process_file* pf;

  for (e = list_begin (file_list); e != list_end (file_list);
       e = list_next (e))
  {
    pf = list_entry (e, struct process_file, file_elem);

    if (pf->fd == fd)
      return pf;
  }
  return NULL; 
}













