#include "fs.h"
#include "threads.h"
#include "const.h"
#include "file.h"
#include "glo.h"
#include "proto.h"
#include "vnode.h"
#include "fproc.h"
#include <minix/endpoint.h>
#include <minix/vfsif.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>

#define OK 0
#define NOT_OK -1

#define locked_by(vp, lc) ((lc->info&EXCL_LOCKED) && lc->dev == vp->v_dev && lc->inode_nr == vp->v_inode_nr)
#define drop_lock(lc) ((lc->info = (lc->info & EXCL_LOCKED ? lc->info^EXCL_LOCKED : lc->info))) 

#define LOCK_EXCL(lc) if (mutex_lock(&lc->mutex) != 0) { \
	panic("Unable to obtain mutex on excl_lock");}
#define UNLOCK_EXCL(lc) if (mutex_unlock(&lc->mutex) != 0) { \
	panic("Unable to release mutex on excl_lock");}


/* if lock found, return pointer to ALREADY LOCKED excl_lock */
struct excl_lock* find_excl_lock(struct vnode* vp) {
	struct excl_lock* lc;
	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++) {
		LOCK_EXCL(lc);
		if (locked_by(vp, lc)) {
			return lc;
		}
		UNLOCK_EXCL(lc);
	}
	return NULL;
}

/* if free spot found, return pointer to ALREADY LOCKED excl_lock */
struct excl_lock* get_free_excl_lock(void) {
	struct excl_lock* lc;
	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++) {
		LOCK_EXCL(lc);
		if (!(lc->info & EXCL_LOCKED)) {
			return lc;
		}
		UNLOCK_EXCL(lc);
	}
	return NULL;
}

int excl_perm_check(struct vnode* vp, uid_t usr) {
	struct excl_lock* lc;
	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++) {
		LOCK_EXCL(lc);
		if (locked_by(vp, lc)) {
			int r = (lc->owner == usr ? EXCL_OK : EXCL_NOT_OK);
			UNLOCK_EXCL(lc);
			return r;
		}
		UNLOCK_EXCL(lc);
	}
	return EXCL_OK;
}

void excl_drop_lock(struct vnode* vp) {
	struct excl_lock* lc;
	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++) {
		LOCK_EXCL(lc);
		if (locked_by(vp, lc))
			drop_lock(lc);
		UNLOCK_EXCL(lc);
	}
}

void init_excl_locks(void) {
	struct excl_lock* lc;

	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++) {
		if (mutex_init(&lc->mutex, NULL) != 0)
			panic("Failed to initialize filp mutex");
		lc->info = 0;
	}
}

void excl_closing_fd(int closed_fd, pid_t caller_p, struct vnode* vp) {	
	struct excl_lock* lc;
	if ((lc = find_excl_lock(vp)) == NULL)
		return;

	if ((lc->info & EXCL_BY_FD) == EXCL_BY_FD)
		if (lc->caller_p == caller_p && lc->fd == closed_fd)
			drop_lock(lc);
	UNLOCK_EXCL(lc);
}

int check_for_other_users(struct vnode* vp) {
	for (int i = 0; i < NR_PROCS; i++)
		for (int j = 0; j < OPEN_MAX; i++)
			if (fproc[i].fp_realuid == fp->fp_realuid &&
			    fproc[i].fp_filp[j] != NULL &&
			    fproc[i].fp_filp[j]->filp_vno == vp)
			{
				return NOT_OK;
			}
	return OK;
}

int do_locking(struct vnode* vp, int fd, int info) {
	struct excl_lock* lc;

	if (find_excl_lock(vp) != NULL) { /* lock already exists */
		UNLOCK_EXCL(lc);
		return EALREADY;
	}
	if ((lc = get_free_excl_lock()) == NULL) /* there's no space for another lock */
		return ENOLCK;
	
	lc->owner = fp->fp_realuid;
	lc->caller_p = fp->fp_pid;
	lc->fd = fd;
	lc->info = info;
	lc->inode_nr = vp->v_inode_nr;
	lc->dev = vp->v_dev;

	UNLOCK_EXCL(lc);

	return OK;
}

int do_common(struct vnode* vp, int flags, int fd, int info) {
	struct excl_lock* lc;
	uid_t caller = fp->fp_realuid;

	switch (flags) {
		case EXCL_LOCK:
			return do_locking(vp, fd, info);
		case EXCL_LOCK_NO_OTHERS:
			if (check_for_other_users(vp) != OK)
				return EAGAIN;
			return do_locking(vp, fd, info);
		case EXCL_UNLOCK:
			if ((lc = find_excl_lock(vp)) == NULL)
				return EINVAL;
			if (caller != lc->owner) {
				UNLOCK_EXCL(lc);
				return EPERM;
			}
			drop_lock(lc);
			UNLOCK_EXCL(lc);
		case EXCL_UNLOCK_FORCE:
			if ((lc = find_excl_lock(vp)) == NULL)
				return EINVAL;
			if (caller != lc->owner && caller != SU_UID && caller != vp->v_uid) {
				UNLOCK_EXCL(lc);
				return EPERM;
			}
			drop_lock(lc);
			UNLOCK_EXCL(lc);
		default:
			return EINVAL;
	}

	return OK;
}

int do_exclusive(void) {
	int flags = job_m_in.m_lc_vfs_exclusive.flags;
	int fd = -1;
	int info = EXCL_BY_PATH;

	return do_common(NULL,flags, fd, info) ;
}

int do_fexclusive(void) {
	int flags = job_m_in.m_lc_vfs_exclusive.flags;
	int fd = job_m_in.m_lc_vfs_exclusive.fd;
	int info = EXCL_BY_FD;

	struct filp* fil;
	if ((fil = get_filp(fd, TLL_READ)) == NULL)
		return EBADF;

	if (fil->filp_count == 0 || fp->fp_filp[fd]->filp_vno == NULL) {
		unlock_filp(fil);
		return EBADF;
	}
	/* check for r/w permission only if locking file */
	if (!(fil->filp_mode & (R_BIT|W_BIT)) && 
	   (flags & (EXCL_LOCK|EXCL_LOCK_NO_OTHERS)))
	{
		unlock_filp(fil);
		return EBADF;
	}
	int r = do_common(fil->filp_vno, flags, fd, info);
	unlock_filp(fil);
	return r;
}
