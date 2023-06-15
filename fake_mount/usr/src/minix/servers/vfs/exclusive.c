#include "fs.h"
#include "glo.h"
#include "fproc.h"
#include <minix/endpoint.h>
#include <sys/types.h>
#include <stdio.h>

int find_owner(endpoint_t caller, uid_t* owner) {
	printf("caller: %d\n", caller);
	for (int i = 0; i < NR_PROCS; i++)
		if (fproc[i].fp_endpoint == caller) {
			*owner = fproc[i].fp_realuid;
			return 0;
		}
	printf("NOT FOUND\n");
	return ENOSYS;
}

int do_exclusive(void) {
	uid_t owner = 0;
	int error;
	printf("HERE\n");
	if ((error = find_owner(who_e, &owner)))
		return error;
	printf("%d", owner);
	return(ENOSYS);  // TODO: implementacja VFS_EXCLUSIVE
}

int do_fexclusive(void) {
	return(ENOSYS);  // TODO: implementacja VFS_FEXCLUSIVE
}
