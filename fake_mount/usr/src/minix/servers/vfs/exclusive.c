#include "fs.h"
#include "file.h"
#include <minix/com.h>
#include <sys/stat.h>
#include <minix/vfsif.h>
#include <minix/callnr.h>
#include "tll.h"
#include "vnode.h"
#include "path.h"
#include <sys/fcntl.h>
#include <stdio.h>

#define OK 0
#define NOT_OK -1

#define locked_by(vp, lc) ((lc->info&EXCL_LOCKED) && lc->dev == vp->v_dev && lc->inode_nr == vp->v_inode_nr)
#define drop_lock(lc) ((lc->info = ((lc->info & EXCL_LOCKED) ? (lc->info^EXCL_LOCKED) : lc->info))) 

#define LOCK_EXCL(lc) if (mutex_lock(&lc->mutex) != 0) { \
	panic("Unable to obtain mutex on excl_lock");}
#define UNLOCK_EXCL(lc) if (mutex_unlock(&lc->mutex) != 0) { \
	panic("Unable to release mutex on excl_lock");}

void printf_occ() {
	int ile = 0;
	struct excl_lock* lc;
	for (lc = &excl_lock[0]; lc < &excl_lock[NR_EXCLUSIVE]; lc++)
		if (lc->info&EXCL_LOCKED)
			ile++;
      printf("Zajete jest %d\n", ile);
}


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

void excl_put_vnode_handler(struct vnode* vp) {
	struct excl_lock* lc;
	if ((lc = find_excl_lock(vp)) == NULL)
		return;
	if (lc->info & EXCL_MOVED)
		drop_lock(lc);
	UNLOCK_EXCL(lc);
}

void excl_mark_as_moved(struct vnode* vp) {
	struct excl_lock* lc;
	if ((lc = find_excl_lock(vp)) == NULL)
		return;

	lc->info |= EXCL_MOVED;
	UNLOCK_EXCL(lc);
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

	if ((lc = find_excl_lock(vp)) != NULL) { /* lock already exists */
		UNLOCK_EXCL(lc);
		return EALREADY;
	}
	if ((lc = get_free_excl_lock()) == NULL) /* there's no space for another lock */
		return ENOLCK;
	
	lc->owner = fp->fp_realuid;
	lc->caller_p = fp->fp_pid;
	lc->fd = fd;
	lc->info = info&EXCL_LOCKED;
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
			break;
		case EXCL_UNLOCK_FORCE:
			if ((lc = find_excl_lock(vp)) == NULL)
				return EINVAL;
			if (caller != lc->owner && caller != SU_UID && caller != vp->v_uid) {
				UNLOCK_EXCL(lc);
				return EPERM;
			}
			drop_lock(lc);
			UNLOCK_EXCL(lc);
			break;
		default:
			return EINVAL;
	}

	return OK;
}

int do_exclusive(void) {
	printf_occ();
	int flags = job_m_in.m_lc_vfs_exclusive.flags;
	int fd = -1;
	int info = EXCL_BY_PATH;
	vir_bytes name = job_m_in.m_lc_vfs_exclusive.name;
	size_t len = job_m_in.m_lc_vfs_exclusive.len;
	char fullpath[PATH_MAX];
	int r;
	
	struct vnode* vp;
	struct vmnt* vmp;
	struct lookup resolve;

	lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
	resolve.l_vmnt_lock = VMNT_READ;
	resolve.l_vnode_lock = VNODE_READ;

	if (fetch_name(name, len, fullpath) != OK) return(err_code);
	if ((vp = eat_path(&resolve, fp)) == NULL) return(err_code);

	if ((r = forbidden(fp, vp, R_BIT)) != OK &&
	    (r = forbidden(fp, vp, W_BIT)) != OK)
	{
		r = EACCES;
	}
	else 
		r = do_common(vp,flags, fd, info);

	unlock_vnode(vp);
	unlock_vmnt(vmp);

	put_vnode(vp);
	return r;
}

int do_fexclusive(void) {
	printf_occ();
	int flags = job_m_in.m_lc_vfs_exclusive.flags;
	int fd = job_m_in.m_lc_vfs_exclusive.fd;
	int info = EXCL_BY_FD;
	int r;

	struct filp* fil;
	if ((fil = get_filp(fd, TLL_READ)) == NULL)
		return EBADF;

	if ((r = forbidden(fp, fil->filp_vno,R_BIT) != OK) &&
		(r = forbidden(fp, fil->filp_vno, W_BIT) != OK))
	{
		r = EBADF;
	}
	else 
		r = do_common(fil->filp_vno, flags, fd, info);

	unlock_filp(fil);
	return r;
}
