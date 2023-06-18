#include "excl_lock.h"
#include "fs.h"
#include "const.h"
#include "file.h"
#include "glo.h"
#include "vnode.h"
#include "fproc.h"
#include <minix/endpoint.h>
#include <minix/vfsif.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>

#define OK 0
#define NOT_OK -1

struct excl_lock* find_excl_lock(struct vnode* vp) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		if (excl_lock[i].vp == vp)
			return &excl_lock[i];
	return NULL;
}

struct excl_lock* get_free_excl_lock(void) {
	return find_excl_lock(NULL);
}

int excl_perm_check(struct vnode* vp, uid_t usr) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		if (excl_lock[i].vp == vp)
			return (excl_lock[i].owner == usr ? EXCL_OK : EXCL_NOT_OK);
	return EXCL_OK;
}

void excl_drop_lock(struct vnode* vp) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		if (excl_lock[i].vp == vp)
			excl_lock[i].vp = NULL;
}

void init_excl_locks(void) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		excl_lock[i].vp = NULL;
}

void excl_closing_fd(int closed_fd, pid_t caller_p, struct vnode* vp) {	
	struct excl_lock* lc;
	if ((lc = find_excl_lock(vp)) == NULL)
		return;

	if ((lc->info & EXCL_BY_FD) == EXCL_BY_FD)
		if (lc->caller_p == caller_p && lc->fd == closed_fd)
			lc->vp = NULL; /* drop lock */
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

int do_locking(struct vnode* vp, int flags, int fd, int info) {
	struct excl_lock* lc;

	if (find_excl_lock(vp) != NULL) /* lock already exists */
		return EALREADY;
	if ((lc = get_free_excl_lock()) == OK) /* there's no space for another lock */
		return ENOLCK;
	
	lc->vp = vp;
	lc->owner = fp->fp_realuid;
	lc->caller_p = fp->fp_pid;
	lc->fd = fd;
	lc->info = info;

	return OK;
}

int do_common(struct vnode* vp, int flags, int fd, int info) {
	struct excl_lock* lc;
	uid_t caller = fp->fp_realuid;

	switch (flags) {
		case EXCL_LOCK:
			return do_locking(vp, flags, fd, info);
		case EXCL_LOCK_NO_OTHERS:
			if (check_for_other_users(vp) != OK)
				return EAGAIN;
			return do_locking(vp, flags, fd, info);
		case EXCL_UNLOCK:
			if ((lc = find_excl_lock(vp)) == NULL)
				return EINVAL;
			if (caller != lc->owner)
				return EPERM;
			lc->vp = NULL;
		case EXCL_UNLOCK_FORCE:
			if ((lc = find_excl_lock(vp)) == NULL)
				return EINVAL;
			if (caller != lc->owner && caller != SU_UID && caller != vp->v_uid)
				return EPERM;
			lc->vp = NULL;
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

	if (fd < 0 || fd >= OPEN_MAX)
		return EBADF;
	if (fp->fp_filp[fd] == NULL)
		return EBADF;
	if (fp->fp_filp[fd]->filp_count == 0 || fp->fp_filp[fd]->filp_vno == NULL)
		return EBADF;
	/* check for r/w permission only if locing file */
	if (!(fp->fp_filp[fd]->filp_mode & (R_BIT|W_BIT)) && 
	   (flags & (EXCL_LOCK|EXCL_LOCK_NO_OTHERS))
		return EBADF;

	return do_common(fp->fp_filp[fd]->filp_vno, flags, fd, info);
}
