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
#include <synch.h>

struct OF **open_files = NULL;

/*
 * Initialize state required for the open file and file descriptor tables.
 * Called from boot(), so no synchronization is required.
 */
void fs_bootstrap() {
	kprintf("INIT FS...\n"); /* TODO: debug-only */
	num_files = 0;
	open_files = NULL;
	of_lock = lock_create("open file table lock");
	if (of_lock == NULL) panic("Out of memory\n");
}

/*
 * Free memory used by the open file and file descriptor tables.
 * Called from shutdown(), so no synchronization is required.
 */
void fs_clear_tables() {
	kprintf("FREEING FS...\n"); /* TODO: debug-only */
	for (uint32_t i = 0; i < num_files; i++) {
		if (open_files[i]->file_lock != NULL) {
			lock_destroy(open_files[i]->file_lock);
			vfs_close(open_files[i]->v);
		}
		kfree(open_files[i]);
	}
	kfree(open_files);
	lock_destroy(of_lock);
}

/*
 * Check if the fd is valid for the current process.
 */
bool valid_fd(uint32_t fd) {
	if (fd <= 0 || fd >= OPEN_MAX) {
		return false; /* invalid file descriptor */
	} else if (curproc->fds[fd]->free == true) {
		return false; /* file must not be not open */
	}
	return true; /* this is an allocated fd for this process */
}

/*
 * Initialise the fd table for the process.
 */
int init_fd_table() {
	kprintf("INIT FD TABLE...\n"); /* TODO: debug-only */
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
	int ret = copyinstr(path, path_kern, (size_t) PATH_MAX, &path_kern_size);
	if (ret) return ret; /* rest of error-checking handled here */

	kprintf("OPENING FILE...%s %d\n", path_kern, flags); /* TODO: debug-only */
	struct FD **fds = curproc->fds;

	/* look for the first available fd */
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

	/* set can_read/can_write flags according to flags argument */
	fds[fd_found]->can_read = true;
	fds[fd_found]->can_write = true;
	if ((flags & O_RDWR) != 0) {
		kprintf("CAN READ AND WRITE\n"); /* TODO: debug-only */
	} else if ((flags & O_WRONLY) != 0) {
		kprintf("CAN WRITE ONLY\n"); /* TODO: debug-only */
		fds[fd_found]->can_read = false;
	} else if (flags == O_RDONLY) {
		kprintf("CAN READ ONLY\n"); /* TODO: debug-only */
		fds[fd_found]->can_write = false;
	} else {
		kprintf("Invalid flags to open %d\n", flags); /* TODO: debug-only */
		return EINVAL; /* unknown flags */
	}

	/* open the file - will increase the ref count in vnode */
	struct vnode vn;
	struct vnode *v = &vn;
	ret = vfs_open(path_kern, flags, mode, &v);
	if (ret) {
		kprintf("Error being returned from vfs_open %d\n", ret); /* TODO: debug-only */
		return ret; /* rest of error-checking handled here */
	}

	/* create a new entry in open file table for the vnode */
	lock_acquire(of_lock);
	open_files = (struct OF **) krealloc(open_files,
		sizeof(OF *) * num_files, sizeof(OF *) * (num_files + 1));
	open_files[num_files] = kmalloc(sizeof(struct OF));
	open_files[num_files]->offset = 0;
	open_files[num_files]->refcount = 1;
	open_files[num_files]->v = v;
	open_files[num_files]->can_seek = VOP_ISSEEKABLE(v);
	open_files[num_files]->file_lock = lock_create("file lock");
	if (open_files[num_files]->file_lock == NULL) {
		lock_release(of_lock);
		return ENOMEM;
	}
	fds[fd_found]->file = open_files[num_files++];
	lock_release(of_lock);

	/* adjust file pointer in the case of O_APPEND being specified */
	if ((flags & O_APPEND) != 0) {
		struct stat stats;
		VOP_STAT(fds[fd_found]->file->v, &stats);
		fds[fd_found]->file->offset = stats.st_size; /* new entry; no sync needed */
	}

	fds[fd_found]->free = false; /* mark this fd as used */
	*fd = fd_found;
	kprintf("fd returned: %d\n", fd_found); /* TODO: debug-only */
	return 0;
}

/*
 * Close an open file. Return EBADF for an invalid fd.
 */
int sys_close(uint32_t fd) {
	kprintf("CLOSING FILE...%d\n\n", fd); /* TODO: debug-only */
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) return EBADF;

	/* decrement OF refcount - only close file if no more references to the file */
	lock_acquire(fds[fd]->file->file_lock);
	if (--fds[fd]->file->refcount == 0) {
		lock_release(fds[fd]->file->file_lock);

		/* hard i/o error is unlikely and rarely checked - see kern/vfs/vfspath.c */
		vfs_close(fds[fd]->file->v);

		lock_destroy(fds[fd]->file->file_lock);
		kfree(fds[fd]->file);

		/* prevent fs_clear_tables from calling vfs_open and lock_destroy again */
		fds[fd]->file->file_lock = NULL;
	} else {
		lock_release(fds[fd]->file->file_lock);
	}

	fds[fd]->free = true; /* fd can be re-used for this process */
	return 0;
}

/*
 * Read up to buflen bytes into the buffer buf and return number of bytes read.
 */
int sys_read(uint32_t fd, const_userptr_t buf, size_t buflen, size_t *read) {
	kprintf("READING FILE...%d %d\n", fd, buflen); /* TODO: debug-only */
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) {
		return EBADF;
	} else if (!fds[fd]->can_read) {
		return EBADF; /* file was not opened for reading */
	}

	struct iovec iov;
	struct vnode *v = fds[fd]->file->v;
	struct uio u;

	char buf_kern[PATH_MAX] = {0};
	lock_acquire(fds[fd]->file->file_lock);
	uio_kinit(&iov, &u, buf_kern, buflen, fds[fd]->file->offset, UIO_READ);
	lock_release(fds[fd]->file->file_lock);

	size_t resid = u.uio_resid;
	lock_acquire(of_lock); /* ensure reads are atomic */
	int ret = VOP_READ(v, &u); /* read vnode contents into buf_kern */
	lock_release(of_lock);
	if (ret) return ret; /* rest of error-checking handled here */

	/* copy data from kernel buffer into user buffer */
	if (resid - u.uio_resid > 0) {
		ret = copyout(buf_kern, (userptr_t) buf, resid - u.uio_resid);
		if (ret) return ret; /* rest of error-checking handled here */
	}

	/* advance the file offset */
	lock_acquire(fds[fd]->file->file_lock);
	fds[fd]->file->offset += resid - u.uio_resid;
	lock_release(fds[fd]->file->file_lock);

	/* TODO: debug-only */
	/*kprintf("(kernel) BUF CONTENTS:\n");
	kprintf("###################\n");
	kprintf("%s", (char *) buf_kern);
	kprintf("###################\n");*/

	*read = resid - u.uio_resid;
	return 0;
}

/*
 * Write up to nbytes bytes to a file and return number of bytes written.
 */
int sys_write(uint32_t fd, const_userptr_t buf, size_t nbytes, size_t *written) {
	/* copy user space pointer to kernel space buffer */
	char buf_kern[PATH_MAX] = {0};
	if (nbytes > 0) {
		int ret = copyin(buf, buf_kern, nbytes);
		if (ret) return ret; /* rest of error-checking handled here */
	}

	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) {
		return EBADF;
	} else if (!fds[fd]->can_write) {
		return EBADF; /* file was not opened for writing */
	}

	struct iovec iov;
	iov.iov_kbase = buf_kern;
	iov.iov_len = nbytes;

	struct vnode *v = fds[fd]->file->v;
	struct uio u;
	lock_acquire(fds[fd]->file->file_lock);
	uio_kinit(&iov, &u, buf_kern, nbytes, fds[fd]->file->offset, UIO_WRITE);
	lock_release(fds[fd]->file->file_lock);

	size_t resid = u.uio_resid;
	lock_acquire(of_lock); /* ensure writes are atomic */
	int ret = VOP_WRITE(v, &u);
	lock_release(of_lock);
	if (ret) return ret; /* rest of error-checking handled here */

	/* advance the file offset */
	lock_acquire(fds[fd]->file->file_lock);
	fds[fd]->file->offset += resid - u.uio_resid;
	lock_release(fds[fd]->file->file_lock);

	*written = resid - u.uio_resid;
	return 0;
}

/*
 * Change the file pointer offset of an open file.
 */
int sys_lseek(uint32_t fd, off_t pos, int whence, off_t *ret) {
	kprintf("SEEKING FILE...%d %lld %d\n", fd, pos, whence); /* TODO: debug-only */
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd)) return EBADF;

	/* only proceed if underlying file object is seekable */
	if (!fds[fd]->file->can_seek) {
		return ESPIPE;
	}

	lock_acquire(fds[fd]->file->file_lock);
	off_t old_offset = fds[fd]->file->offset;
	if (whence == SEEK_SET) {
		fds[fd]->file->offset = pos;
	} else if (whence == SEEK_CUR) {
		fds[fd]->file->offset += pos;
	} else if (whence == SEEK_END) {
		struct stat stats;
		lock_acquire(of_lock); /* another process may change file size */
		VOP_STAT(fds[fd]->file->v, &stats);
		lock_release(of_lock);
		fds[fd]->file->offset = stats.st_size + pos;
	} else {
		lock_release(fds[fd]->file->file_lock);
		return EINVAL; /* unknown whence */
	}

	/* revert to old offset if new offset < 0 */
	if (fds[fd]->file->offset < 0) {
		fds[fd]->file->offset = old_offset;
		lock_release(fds[fd]->file->file_lock);
		return EINVAL;
	}

	*ret = fds[fd]->file->offset;
	lock_release(fds[fd]->file->file_lock);
	return 0;
}

/*
 * Clone file handles.
 */
int sys_dup2(int32_t oldfd, int32_t newfd, int32_t *retfd) {
	kprintf("(kern) DUPLICATING FD...old:%d new:%d\n", oldfd, newfd); /* TODO: debug-only */
	kprintf("(kern) OLD: %d, NEW: %d\n", oldfd, newfd); /* TODO: debug-only */
	if (!valid_fd(oldfd) || newfd < 0 || newfd >= OPEN_MAX) return EBADF;

	/* do nothing if both FDs are identical */
	if (oldfd != newfd) {
		struct FD **fds = curproc->fds;

		/* newfd refers to an already open file descriptor - close it */
		if (!fds[newfd]->free) sys_close(newfd);

		/* clone properties of oldfd onto newfd and increment refcount */
		lock_acquire(fds[oldfd]->file->file_lock);
		fds[oldfd]->file->refcount++;
		lock_release(fds[oldfd]->file->file_lock);
		fds[newfd]->file = fds[oldfd]->file;
		fds[newfd]->free = false;
		fds[newfd]->can_read = fds[oldfd]->can_read;
		fds[newfd]->can_write = fds[oldfd]->can_write;
	}

	*retfd = newfd;
	return 0;
}
