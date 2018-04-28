#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>
#include <proc.h>

struct OF *open_files = NULL;

/*
 * Initialize state required for the open file and file descriptor tables.
 */
void fs_bootstrap() {
	kprintf("\nINIT FS...\n");
	num_files = 0;
	open_files = NULL;
}

/*
 * Free memory used by the open file and file descriptor tables.
 */
void fs_clear_tables() {
	kprintf("\nFREEING FS...\n");
	kfree(open_files);
}

/*
 * Check if the fd is valid for the current process.
 */
bool valid_fd(uint32_t fd, struct FD *fds) {
	if (fd <= 0 || fd >= __OPEN_MAX) {
		return false; /* invalid file descriptor */
	}
	if (fds == NULL) {
		return false; /* this process has no open files */
	}
	return true;
}

//TODO: in proc_end, kfree the fd table

/*
 * Initialise the fd table for the process.
 */
void init_fd_table() {
	struct FD **fds = kmalloc(sizeof(struct FD *) * __OPEN_MAX);
	KASSERT(fds != NULL);
	curproc->fds = fds;
	for (int i = 0 ; i < __OPEN_MAX; ++i) {
		curproc->fds[i] = kmalloc(sizeof(FD));
		KASSERT(curproc->fds[i] != NULL);
		curproc->fds[i]->free = true;
	}
	curproc->fds[0]->free = false; /* skip stdin fd, so stdout/err will
	be opened on fd 1 and 2 respectively */
}

/*
 * Open a file in the specified mode. Return a process-unique file
 * descriptor on success; otherwise, return an appropriate errno.
 */
int sys_open(const_userptr_t path, uint32_t flags, mode_t mode, int *fd) {
	/* copy user space pointer to kernel space buffer */
	char path_kern[PATH_MAX];
	size_t path_kern_size = 0;
	copyinstr(path, path_kern, (size_t) PATH_MAX, &path_kern_size);

	kprintf("\nOPENING FILE...%s %d\n", path_kern, flags);
	struct FD **fds = curproc->fds;

	/* look for an available fd */
	int fd_found = -1;
	for (int i = 0; i < __OPEN_MAX; i++) {
		if (fds[i]->free) {
			fd_found = i;
			break;
		}
	}
	if (fd_found == -1) {
		kprintf("Ran out of file descriptors\n");
		return EMFILE; /* reached max open files for this process */
	}

	/* set can_read/can_write flags according to flags argument */
	if ((flags & O_RDWR) != 0) {
		kprintf("CAN READ AND WRITE\n");
		fds[fd_found]->can_read = true;
		fds[fd_found]->can_write = true;
	} else if ((flags & O_WRONLY) != 0) {
		kprintf("CAN WRITE ONLY\n");
		fds[fd_found]->can_read = false;
		fds[fd_found]->can_write = true;
	} else if (flags == O_RDONLY) {
		kprintf("CAN READ ONLY\n");
		fds[fd_found]->can_read = true;
		fds[fd_found]->can_write = false;
	} else {
		kprintf("Invalid flags to open %d\n", flags);
		return EINVAL; /* unknown flags */
	}

	/* TODO: implement more error handling before calling vfs_open */

	/* open the file - will increase the ref count in vnode */
	struct vnode *v;
	int result = vfs_open(path_kern, flags, mode, &v);
	if (result != 0) {
		kprintf("Error being returned from vfs_open %d\n", result);
		return result;
	}

	/* look for vnode in open file table */
	fds[fd_found]->file = NULL;
	for (uint32_t i = 0; i < num_files; ++i) {
		if (open_files[i].v == v) {
			fds[fd_found]->file = &open_files[i];
			break;
		}
	}

	/* otherwise, create a new entry in open file table for the vnode */
	if (fds[fd_found]->file == NULL) {
		open_files = (struct OF *) krealloc(open_files,
			sizeof(OF) * num_files, sizeof(OF) * (num_files + 1));
		open_files[num_files].offset = 0;
		open_files[num_files].v = v;
		fds[fd_found]->file = &open_files[num_files++];
	}

	fds[fd_found]->free = false; /* mark this fd as used */
	*fd = fd_found;
	return 0;
}

/*
 * Close an open file. Return EBADF for an invalid fd.
 */
int sys_close(uint32_t fd) {
	kprintf("\nCLOSING FILE...%d\n", fd);
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd, *fds)) {
		return EBADF;
	}
	if (fds[fd]->free == false) {
		vfs_close((fds[fd]->file)->v); /* hard i/o error is unlikely and
		rarely checked - see kern/vfs/vfspath.c */
		fds[fd]->free = true; /* fd can be re-used for this process */
		return 0;
	}
	return EBADF; /* fd must not be open */
}

/*
 * Read up to buflen bytes into the buffer buf and return number of bytes read.
 */
int sys_read(uint32_t fd, const_userptr_t buf, size_t buflen, size_t *read) {
	/* copy user space pointer to kernel space buffer */
	char buf_kern[NAME_MAX];
	size_t buf_kern_size = 0;
	copyinstr(buf, buf_kern, buflen, &buf_kern_size);

	kprintf("\nREADING FILE...%d %d\n", fd, buflen);

	struct FD **fds = curproc->fds;
	if (!valid_fd(fd, *fds)) {
		return EBADF;
	} else if (fds[fd]->free == true) {
		return EBADF; /* file must not be not open */
	} else if (!fds[fd]->can_read) {
		return EBADF; /* file was not opened for reading */
	}

	struct iovec iov;
	iov.iov_kbase = buf_kern;
	iov.iov_len = buflen;

	struct vnode *v = fds[fd]->file->v;
	struct uio u;
	uio_kinit(&iov, &u, buf_kern, buflen, fds[fd]->file->offset, UIO_READ);

	size_t resid = u.uio_resid;
	VOP_READ(v, &u);

	/* copy data from kernel buffer into user buffer */
	//copyoutstr(buf_kern, buf, buflen, &buf_kern_size); //TODO

	/* advance the file offset */
	fds[fd]->file->offset += resid - u.uio_resid;

	// TODO ERROR CHECK

	// TODO REMOVE THIS JUST SHOWS READ SUCCEEDED
	kprintf("(kernel) BUF CONTENTS: \n");
	kprintf("###################\n");
	buf_kern[resid - u.uio_resid] = '\0';
	kprintf("%s", buf_kern);
	kprintf("###################\n");

	//TODO: connect stdout/err to console to support printing in user prog

	*read = resid - u.uio_resid;
	return 0;
}

int sys_write(uint32_t fd, const_userptr_t buf, size_t nbytes, size_t *written) {
	/* copy user space pointer to kernel space buffer */
	char buf_kern[NAME_MAX];
	size_t buf_kern_size = 0;
	copyinstr(buf, buf_kern, nbytes, &buf_kern_size);

	kprintf("\nWRITING FILE...%d %s %d\n", fd, buf_kern, nbytes);
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd, *fds)) {
		return EBADF;
	} else if (fds[fd]->free == true) {
		return EBADF; /* file must not be not open */
	} else if (!fds[fd]->can_write) {
		return EBADF; /* file was not opened for writing */
	}

	struct iovec iov;
	iov.iov_kbase = buf_kern;
	iov.iov_len = nbytes;

	struct vnode *v = fds[fd]->file->v;
	struct uio u;
	uio_kinit(&iov, &u, buf_kern, nbytes, fds[fd]->file->offset, UIO_WRITE);
	size_t resid = u.uio_resid;
	VOP_WRITE(v, &u);

	/* advance the file offset */
	fds[fd]->file->offset += resid - u.uio_resid;

	// TODO ERROR CHECK

	*written = resid - u.uio_resid;
	return 0;
}
