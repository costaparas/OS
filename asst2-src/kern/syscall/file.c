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

struct OF **open_files = NULL;

/*
 * Initialize state required for the open file and file descriptor tables.
 */
void fs_bootstrap() {
	kprintf("\nINIT FS...\n"); /* TODO: debug-only */
	num_files = 0;
	open_files = NULL;
}

/*
 * Free memory used by the open file and file descriptor tables.
 */
void fs_clear_tables() {
	kprintf("\nFREEING FS...\n"); /* TODO: debug-only */
	for (uint32_t i = 0; i < num_files; i++) {
		kfree(open_files[i]);
	}
	kfree(open_files);
}

/*
 * Check if the fd is valid for the current process.
 */
bool valid_fd(uint32_t fd) {
	if (fd <= 0 || fd >= OPEN_MAX) {
		return false; /* invalid file descriptor */
	}
	return true;
}

/*
 * Initialise the fd table for the process.
 */
int init_fd_table() {
	struct FD **fds = kmalloc(sizeof(struct FD *) * OPEN_MAX);
	if (fds == NULL) return ENOMEM;
	curproc->fds = fds;
	for (int i = 0 ; i < OPEN_MAX; ++i) {
		curproc->fds[i] = kmalloc(sizeof(FD));
		if (curproc->fds[i] == NULL) return ENOMEM;
		curproc->fds[i]->free = true;
	}
	curproc->fds[0]->free = false; /* skip stdin fd, so stdout/err will
	be opened on fd 1 and 2 respectively */
	return 0;
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

	kprintf("\nOPENING FILE...%s %d\n", path_kern, flags); /* TODO: debug-only */
	struct FD **fds = curproc->fds;

	/* look for an available fd */
	int fd_found = -1;
	for (int i = 0; i < OPEN_MAX; i++) {
		if (fds[i]->free) {
			fd_found = i;
			break;
		}
	}
	if (fd_found == -1) {
		kprintf("Ran out of file descriptors\n"); /* TODO: debug-only */
		return EMFILE; /* reached max open files for this process */
	}

	/* TODO: handle O_APPEND and O_TRUNC flag by setting offset */

	/* set can_read/can_write flags according to flags argument */
	if ((flags & O_RDWR) != 0) {
		kprintf("CAN READ AND WRITE\n"); /* TODO: debug-only */
		fds[fd_found]->can_read = true;
		fds[fd_found]->can_write = true;
	} else if ((flags & O_WRONLY) != 0) {
		kprintf("CAN WRITE ONLY\n"); /* TODO: debug-only */
		fds[fd_found]->can_read = false;
		fds[fd_found]->can_write = true;
	} else if (flags == O_RDONLY) {
		kprintf("CAN READ ONLY\n"); /* TODO: debug-only */
		fds[fd_found]->can_read = true;
		fds[fd_found]->can_write = false;
	} else {
		kprintf("Invalid flags to open %d\n", flags); /* TODO: debug-only */
		return EINVAL; /* unknown flags */
	}

	/* open the file - will increase the ref count in vnode */
	struct vnode *v;
	int result = vfs_open(path_kern, flags, mode, &v);
	if (result != 0) {
		kprintf("Error being returned from vfs_open %d\n", result); /* TODO: debug-only */
		return result; /* rest of error-checking handled here */
	}

	/* look for vnode in open file table */
	fds[fd_found]->file = NULL;
	for (uint32_t i = 0; i < num_files; ++i) {
		if (open_files[i]->v == v) {
			fds[fd_found]->file = open_files[i];
			break;
		}
	}

	/* otherwise, create a new entry in open file table for the vnode */
	if (fds[fd_found]->file == NULL) {
		open_files = (struct OF **) krealloc(open_files,
			sizeof(OF *) * num_files, sizeof(OF *) * (num_files + 1));
		open_files[num_files] = kmalloc(sizeof(struct OF));
		open_files[num_files]->offset = 0;
		open_files[num_files]->v = v;
		fds[fd_found]->file = open_files[num_files++];
	}

	fds[fd_found]->free = false; /* mark this fd as used */
	*fd = fd_found;
	return 0;
}

/*
 * Close an open file. Return EBADF for an invalid fd.
 */
int sys_close(uint32_t fd) {
	kprintf("\nCLOSING FILE...%d\n", fd); /* TODO: debug-only */
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) {
		return EBADF;
	}
	if (fds[fd]->free == false) {
		vfs_close((fds[fd]->file)->v); /* hard i/o error is unlikely and
		rarely checked - see kern/vfs/vfspath.c */
		fds[fd]->free = true; /* fd can be re-used for this process */
		fds[fd]->file->offset = 0; /* TODO: double-check this */
		return 0;
	}
	return EBADF; /* fd must not be open */
}

/*
 * Read up to buflen bytes into the buffer buf and return number of bytes read.
 */
int sys_read(uint32_t fd, const_userptr_t buf, size_t buflen, size_t *read) {
	/* copy user space pointer to kernel space buffer */
	kprintf("\nREADING FILE...%d %d\n", fd, buflen); /* TODO: debug-only */

	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) {
		return EBADF;
	} else if (fds[fd]->free == true) {
		return EBADF; /* file must not be not open */
	} else if (!fds[fd]->can_read) {
		return EBADF; /* file was not opened for reading */
	}

	struct iovec iov;
	struct vnode *v = fds[fd]->file->v;
	struct uio u;

	char buf_kern[PATH_MAX] = {0};
	uio_kinit(&iov, &u, buf_kern, buflen, fds[fd]->file->offset, UIO_READ);

	size_t resid = u.uio_resid;
	int ret = VOP_READ(v, &u); /* read vnode contents into buf_kern */
	if (ret) return ret; /* rest of error-checking handled here */

	/* copy data from kernel buffer into user buffer */
	copyout(buf_kern, (userptr_t) buf, resid - u.uio_resid);

	/* advance the file offset */
	fds[fd]->file->offset += resid - u.uio_resid;

	/* TODO: debug-only */
	kprintf("(kernel) BUF CONTENTS: \n");
	kprintf("###################\n");
	kprintf("%s", (char *) buf);
	kprintf("###################\n");

	*read = resid - u.uio_resid;
	return 0;
}

int sys_write(uint32_t fd, const_userptr_t buf, size_t nbytes, size_t *written) {
	/* copy user space pointer to kernel space buffer */
	char buf_kern[PATH_MAX] = {0};
	size_t buf_kern_size = 0;
	copyinstr(buf, buf_kern, nbytes, &buf_kern_size);

	kprintf("\nWRITING FILE...%d %s %d\n", fd, buf_kern, nbytes); /* TODO: debug-only */
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) {
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
	int ret = VOP_WRITE(v, &u);
	if (ret) return ret; /* rest of error-checking handled here */

	/* advance the file offset */
	fds[fd]->file->offset += resid - u.uio_resid;

	*written = resid - u.uio_resid;
	return 0;
}
