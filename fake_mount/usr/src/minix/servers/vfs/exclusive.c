#include "fs.h"
#include "glo.h"
#include "fproc.h"
#include <minix/endpoint.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>

int check_flags(int flags) {
	static valid[] = {EXCL_UNLOCK, EXCL_UNLOCK_FORCE, EXCL_LOCK, EXCL_LOC_NO_OTHERS};
	for (int i = 0; i < 4; i++)
		if (flags == valid[i])
			return (OK);
	return -1;
}

int do_exclusive(void) {
	uid_t caller = fp->fp_realuid;
}

int check_input_fexclusive(int fd, int flags) {
	int er;
	if ((er = check_flags(flags)) != OK)
		return EINVAL;
	if (fd < 0 || fd >= OPEN_MAX)
		return EBADF;
	if (fp->fp_filp[fd] == NULL)
		return EBADF;
	if (fp->fp_filp[fd].filp_count == 0 || fp->fp_filp[fd]->filp_vno == NULL)
		return EBADF;

}

int do_fexclusive(void) {
	uid_t caller = fp->fp_realuid;
	int fd;
	int flags;

	fd = job_m_in.m_lc_vfs_exclusive.fd;
	flags = job_m_in.m_lc_vfs_exclusive.flags;

	if (check_flags(flags) != OK)
		return EINVAL;

	if (fd < 0)
		return EBADF;

	return(ENOSYS);  // TODO: implementacja VFS_FEXCLUSIVE
}
