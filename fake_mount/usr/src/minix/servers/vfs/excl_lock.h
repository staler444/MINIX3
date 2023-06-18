#ifndef __VFS_EXCL_LOCK_H__
#define __VFS_EXCL_LOCK_H__

/* This is the exclusive lock table.
 * Slot free if (info & EXCL_LOCKED) == false
 */

#include "const.h"
#include "threads.h"
#include <sys/types.h>

EXTERN struct excl_lock {
  mutex_t mutex; 
  ino_t inode_nr;
  dev_t dev;
  int fd;
  pid_t caller_p;
  uid_t owner;

  int info;
} excl_lock[NR_EXCLUSIVE];

#define EXCL_BY_FD        1 /* file locked by VFS_FEXCLUSIVE */
#define EXCL_BY_PATH      2 /* file locked by VFS_EXCLUSIVE */
#define EXCL_MOVED        4 /* file moved after VFS_EXCLUSIVE lock */
#define EXCL_LOCKED       8 /* if lock is in use */

#define EXCL_OK           0   /* permission granted */
#define EXCL_NOT_OK       -1  /* permissions not granted */

#endif
