#include "fs.h"
#include "glo.h"
#include "fproc.h"
#include <minix/endpoint.h>
#include <stdio.h>

int find_owner(pid_t caller, uid_t* owner) {
	for (int i = 0; i < NR_PROCS; i++)
		if (fproc[i].fp_pid == caller) {
			*owner = fproc[i].fp_realuid;
			return 0;
		}		
	
	return ENOSYS;
}

int do_exclusive(void) {
	pid_t caller = _ENDPOINT_P(who_e);
	uid_t owner = 0;
	int error;
	printf("HERE");
	if ((error = find_owner(caller, &owner)))
		return error;
	printf("%d", owner);
	return(ENOSYS);  // TODO: implementacja VFS_EXCLUSIVE
}

int do_fexclusive(void) {
	return(ENOSYS);  // TODO: implementacja VFS_FEXCLUSIVE
}
