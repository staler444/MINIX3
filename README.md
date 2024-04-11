## Project Overview
The objective of this project is to enhance the MINIX3 operating system with a user-exclusive file locking mechanism. This system aims to provide a mandatory, user-level file lock that differs significantly from the standard [flock](https://linux.die.net/man/2/flock) system by allowing temporary blocking without altering file attributes.

### Virtual File System (VFS) Server
The VFS Server of MINIX plays a crucial role in this project. It acts as the Virtual File System server, handling all file system operations. For a comprehensive understanding of its internals and operation, refer to the [MINIX wiki on VFS internals](https://wiki.minix3.org/doku.php?id=developersguide:vfsinternals).

### New System Calls: VFS_FEXCLUSIVE and VFS_EXCLUSIVE
Project introduces two novel system calls, `VFS_FEXCLUSIVE` and `VFS_EXCLUSIVE`, processed by the VFS server. These calls facilitate temporary restrictions on file operations for other users, while allowing the locking user to operate without limitations. Operations affected include open, read, write, truncate, move/rename, and remove. Attempts by other users to perform these operations will result in an `EACCES` error.

#### VFS_FEXCLUSIVE System Call
The `VFS_FEXCLUSIVE` call requires a file descriptor and a flag to indicate the action. Supported actions include:
- **EXCL_LOCK**: Locks the file for exclusive use by the caller. Automatically unlocks upon file descriptor closure if not previously unlocked.
- **EXCL_LOCK_NO_OTHERS**: Similar to EXCL_LOCK, but only locks if no other users have the file open. Returns an `EAGAIN` error if this condition is not met.
- **EXCL_UNLOCK**: Unlocks the file. Only the locking user can unlock.
- **EXCL_UNLOCK_FORCE**: Allows unlocking by the file's owner, the locking user, or the superuser (root).

#### VFS_EXCLUSIVE System Call
The `VFS_EXCLUSIVE` call operates with a file path and an action flag, with similar actions to `VFS_FEXCLUSIVE` but based on file paths instead of descriptors.

### Additional Constraints
- Only simple files (not directories or devices) can be locked.
- Locks persist across file moves within the same partition and do not prevent filesystem unmounts.
- File locks are governed by real user IDs, not effective IDs.
- Specific errors are returned for invalid operations or conditions, enhancing security and predictability.

### Project Structure
- **old_mount**: Contains the original MINIX3 source files to be modified, preserving the original structure for patch generation.
- **fake_mount**: Holds the modified source files, allowing comparison with `old_mount` for patch creation.
- **patcher directory**: Includes scripts for patch management (generation, upload, installation, etc.).
- **test directory**: Contains simple test cases to validate the functionality of the enhanced file locking mechanism.
