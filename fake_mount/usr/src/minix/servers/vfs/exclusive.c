#include "fs.h"
#include "glo.h"
#include <minix/endpoint.h>
#include <systemd/sd-login.h>

int do_exclusive(void) {
	pid_t caller = _ENDPOINT_P(who_e);
	uid_t owner;
	int error;
	if (error = sd_pid_get_owner_uid(caller, &owner) < 0) 
		return error;

	return(ENOSYS);  // TODO: implementacja VFS_EXCLUSIVE
}

int do_fexclusive(void) {
	return(ENOSYS);  // TODO: implementacja VFS_FEXCLUSIVE
}
