#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <list.h>
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

typedef int tid_t;

// Ingyo: define struct process.

struct process
  {
    struct semaphore* wait_sema;
    struct list_elem child_elem;
    int exit_status;
    int fd;
    struct list files;
    tid_t pid;
    int is_exit;
    int is_load;
    struct file* exec_file;
  };

struct process_file
  {
    struct file* file;
    int fd;
    struct list_elem file_elem;
  };

struct lock filesys_lock;

struct process* process_init (void);
void process_remove (struct process* process);
int process_file_init (struct file* file);
void process_file_remove (struct process_file* pf);

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
