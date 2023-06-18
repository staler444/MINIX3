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

/* extracted message values */
uid_t caller;
struct vnode* vp;
int fd, flags, info;

struct excl_lock* lc;

int get_free_excl_lock(void) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		if (excl_lock[i].vp == NULL) {
			lc = &excl_lock[i];
			return OK;
		}
	return NOT_OK;
}

int find_excl_lock(void) {
	for (int i = 0; i < NR_EXCLUSIVE; i++)
		if (excl_lock[i].vp == vp) {
			lc = &excl_lock[i];
			return OK;
		}
	return NOT_OK;
}

int check_for_other_users(void) {
	for (int i = 0; i < NR_PROCS; i++)
		for (int j = 0; j < OPEN_MAX; i++)
			if (fproc[i].fp_realuid == caller &&
				fproc[i].fp_filp[j] != NULL &&
				fproc[i].fp_filp[j]->filp_vno == vp)
			{
				return NOT_OK;
			}
	return OK;
}

int do_locking(void) {
	if (find_excl_lock() != NOT_OK)
		return EALREADY;
	if (get_free_excl_lock() != OK)
		return ENOLCK;
	
	lc->vp = vp;
	lc->owner = caller;
	lc->caller_p = who_p;
	lc->fd = fd;
	lc->info = info;

	return OK;
}

int do_common(void) {
	switch (flags) {
		case EXCL_LOCK:
			return do_locking();
		case EXCL_LOCK_NO_OTHERS:
			if (check_for_other_users() != OK)
				return EAGAIN;
			return do_locking();
		case EXCL_UNLOCK:
			if (find_excl_lock() != OK)
				return EINVAL;
			if (caller != lc->owner)
				return EPERM;
			lc->vp = NULL;
		case EXCL_UNLOCK_FORCE:
			if (find_excl_lock() != OK)
				return EINVAL;
			if (caller != lc->owner && caller != SU_UID)
				return EPERM;
			lc->vp = NULL;
		default:
			return EINVAL;
	}

	return OK;
}

int do_exclusive(void) {
	caller = fp->fp_realuid;

	return EPERM;
}

int do_fexclusive(void) {
	caller = fp->fp_realuid;
	fd = job_m_in.m_lc_vfs_exclusive.fd;
	flags = job_m_in.m_lc_vfs_exclusive.flags;
	info = EXCL_BY_FD;

	if (fd < 0 || fd >= OPEN_MAX)
		return EBADF;
	if (fp->fp_filp[fd] == NULL)
		return EBADF;
	if (fp->fp_filp[fd]->filp_count == 0 || fp->fp_filp[fd]->filp_vno == NULL)
		return EBADF;
	if (!(fp->fp_filp[fd]->filp_mode & (R_BIT|W_BIT)))
		return EBADF;

	vp = fp->fp_filp[fd]->filp_vno;
	
	return do_common();
}
