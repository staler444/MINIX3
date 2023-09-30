# User's exclusive file lock
Project's goal is to extend vfs server by user's exclusive file lock. Unlike standard flock, this locking mechanism will be mandatory, and will work at user's level (not precesses). Unlike standard file premissions, this mechanism will also implement temporary blocking, without need to change files atributes.

# VFS server
MINIX's Virtual File System server. For detailed description see [MINIX wiki](https://wiki.minix3.org/doku.php?id=developersguide:vfsinternals).

# VFS_FEXCLUSIVE and VFS_EXCLUSIVE system calls

New blocking mechanism will depend on two new system calls handled by vfs server, VFS_FEXCLUSIVE and VFS_EXCLUSIVE.
By them, user will be able to temporarly, on the indicated file, prevent other users to perform operations listet down below:
+ open (VFS_OPEN and VFS_CREATE syscalls)
+ read (VFS_READ)
+ write (VFS_WRITE)
+ truncate (VFS_TRUNCATE and VFS_FTRUNCATE)
+ move and rename file (VFS_RENAME both cases, when blocked file is first or second argument of syscall)
+ remove (VFS_UNLINK)

User who blocked file can perform above operations without any limitations, from difrent processes. Attempt to perform any of them by other users will end with EACESS error.

## VFS_FEXCLUSIVE

VFS_FEXCLUSIVE system call takes two arguments, file descriptor and flag indicating action to perform. Following actions are supported:
+ EXCL_LOCK: Lock file indicated by file descriptor, to exclusive use for user performing call. If file wasn't unlocked earlier, then file will by automaticly unlocked at the moment of closing file descriptor.
+ EXCL_LOCK_NO_OTHERS: Works same as EXCL_LOCK, but locks file ONLY if file is NOT open by any OTHER user at the moment (user who perform call, can have this file opened). If this conditions are not met, syscall ends with EAGAIN error.
+ EXCL_UNLOCK: Unlocks file indicated by file descriptor. File can by unlocked only by user, who locked it.
+ EXCL_UNLOCK_FORCE: Unlocks file indicated by file descriptor. File can be unlocked by user who locked it, by user who is owner of that file or by superuser (aka root, aka UID=0).

## VFS_EXCLUSIVE

VFS_EXCLUSIVE system call takes two arguments, path to file and dlag indicating action to perform. Following actions are supported:

+ EXCL_LOCK: Lock file indicated by path, to exclusive use for user performing call. Files stays locked until it's excplicitly unlocked by user. Only exception is when locked file is removed by user (by VFS_UNLINK) or replaced (locked file would be second argumnet of VFS_RENAME). In this case file will by automaticly unlocked when non process will have file opened (file became unused at the moment).

+ EXCL_LOCK_NO_OTHERS: Works as EXCL_LOCK, but file is locked only if no other user have file opened (only caller user have is using file at the moment). If this condidion isn't met, call ends with EAGAIN error.

+ EXCL_UNLOCK: Unlocks file indicated by path. File can be unlocked only by user, who locked it.

+ EXCL_UNLOCK_FORCE: Unlocks file indicated by path. File can be unlocked by user who locked it, by user who is owner of that file or by superuser (aka root, aka UID=0).

## Additional constraints

+ Only simple files are locked. Attempt to lock directory/pseudodevice/pipe/fifi etc. will end with EFTYPE error.

+ Always actual file is locked, indicated by file descriptor/path at the moment of call. File doesn't stop being locked after move (within same partition), rename, after accesing via link path. Techicaly actual v-node/i-node is being locked.

+ Files locks aren't held after file system unmount. Presence of locked file doesn't interfere with unmounting (if unmounting partition would end with succes without locked files, will end with succes with them).

+ Lock on file is held from the moment of locking to the moment of unlocking. Listet above system calls check users permissions to access file before every call, so following scenario is posible: User A with sucess opens file. User B locks file. User B try to read file, ends with EACESS error. User A unlock file. User B's another try to read ends with succes.

+ Users are identified by thier real id (real UID), regardles of thier effective id (effective UID).

+ With VFS_FEXCLUSIVE user can only lock file if provided descriptor is open in read or write mode (or both). If not, call ends with EBADF error. 

+ With VFS_EXCLUSIVE user can only lock file if have rights to read/write that file. If not, call ends with EACCES error.

+ VFS_FEXCLUSIVE fail with EBADF if provided descriptor is not valid.

+ Both VFS_FEXCLUSIVE and VFS_EXCLUSIVE fail with EINVAL error, if provided flag is incorrect, or file to unlock is not locked.

+ Both $\textcolor{red}{VFS\_FEXCLUSIVE}$ and VFS_EXCLUSIVE fail with EALREADY error, if user try --(s?) to lock already locked file.

+ Both VFS_FEXCLUSIVE and VFS_EXCLUSIVE fail with EPERM error, if user try to unlock file, which he is not permited to.

+ There are only NR_EXCLUSIVE simutanusy (?) locked files. If there are already NR_EXCLUSIVE locked files, every attempt to lock file will fail with ENOLCK error.

+ VFS_FEXCLUSIVE and VFS_EXCLUSIVE provide interfase to same locking mechanism. User can lock file via VFS_FEXCLUSIVE and unlock it via VFS_EXCLUSIVE and vice versa (?).

# Project's structure

## old_mount

Contains original file tree with structure of original minix source files, but contains only files that are to by chaned by patch. Preserving original minix source file structure enables to generate patches eazly.

## fake_mount 
Same files as old_mount, but with changes. These are actual source file of tis project. Comparing these with old_mount generates patches.

## patcher directory

Contains scripts to generate/upload/install patches etc.

## test directory 

Contains few simple test cases.

$\mathcal{\color{purple}{this \ is \ a \ paragraph} \ \color{cyan}{in \ another \ font}}$

$\mathbb{\color{teal}{this \ is \ a } \ \color{magenta}{paragraph \ in \ another \ font}}$

$\mathscr{\color{red}{this} \ \ \color{blue}{is \ \ a \ \ paragraph} \ \ \color{yellow}{in \ \ another \ \ font}}$

$\mathfrak{\color{lime}{this \ is \ a \ paragraph \ in \ another \ font}}$

$\mathscr{\color{red}{mon}\color{white}{day}}$

$\textcolor{olive}{\TeX} \ \textcolor{darkgray}{workaround \ found \ by \ Dassalem \ Mohammed \ Yasser}$

$\textit{hello}$  #italic

$\text{hello}$    #normal

$\Large{hello}$$   #Bigger text size

