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

/*
 * Initialize state required for the open file and file descriptor tables.
 */
void fs_bootstrap() {
	kprintf("\nINIT FS...\n");
	num_files = 0;
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
	if (fd >= __OPEN_MAX) {
		return false; /* invalid file descriptor */
	}
	if (fds == NULL) {
		return false; /* this process has no open files */
	}
	return true;
}

/*
 * Increase size of a buffer (crude version of krealloc).
 */
void *expand_buffer(void *src, uint32_t size, uint32_t new_size) {
	void *tmp = kmalloc(new_size);
	KASSERT(tmp != NULL);
	memcpy(tmp, src, size);
	kfree(src);
	return tmp;
}

//TODO: in proc_end, kfree the fd table

void init_fd_table() {
	struct FD *fds = kmalloc(sizeof(FD) * __OPEN_MAX);
	KASSERT(fds != NULL);
	curproc->fds = &fds;
	KASSERT(curproc->fds != NULL);
	for (int i = 0 ; i < __OPEN_MAX; ++i) {
		curproc->fds[i]->free = true;
	}
}

/*
 * Open a file in the specified mode. Return a process-unique file
 * descriptor on success; otherwise, return an appropriate errno.
 */
int sys_open(void *path, uint32_t flags, mode_t mode) {
	kprintf("\nOPENING FILE...%s %d\n", (char *) path, flags);
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
		return EMFILE; /* reached max open files for this process */
	}

	// Check whether O_WRONLY or O_RDWR in flags; update can_write flag if true
	if ((flags & O_WRONLY) != 0 || (flags & O_RDWR) != 0) {
		kprintf("CAN_WRITE FLAG ENABLED!\n");
		fds[fd_found]->can_write = true;
	} else {
		fds[fd_found]->can_write = false;
	}

	/* TODO: implement more error handling before calling vfs_open */

	/* open the file - will increase the ref count in vnode */
	struct vnode *v;
	int result = vfs_open(path, flags, mode, &v);
	if (result != 0) return result;

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
		open_files = (struct OF *) expand_buffer(open_files,
			sizeof(OF) * num_files, sizeof(OF) * (num_files + 1));
		open_files[num_files].offset = 0;
		open_files[num_files].v = v;
		fds[fd_found]->file = &open_files[num_files++];
	}

	fds[fd_found]->free = false; /* mark this fd as used */
	return fd_found;
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
int sys_read(uint32_t fd, void *buf, size_t buflen) {
	kprintf("\nREADING FILE...%d %s %d\n", fd, (char *) buf, buflen);
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd, *fds)) {
		return EBADF;
	} else if (fds[fd]->free == true) {
		return EBADF; /* file must not be not open */
	}

	struct iovec iov;
	iov.iov_kbase = buf;
	iov.iov_len = buflen;

	struct vnode *v = fds[fd]->file->v;
	struct uio u;
	uio_kinit(&iov, &u, buf, buflen, fds[fd]->file->offset, UIO_READ);

	VOP_READ(v, &u);

	// Advance the file offset
	// TODO THIS ASSUMES THAT BUFLEN BYTES ARE READ - NOT ALWAYS THE CASE
	fds[fd]->file->offset += strlen(buf); //TODO: check this

	// TODO ERROR CHECK

	// TODO REMOVE THIS JUST SHOWS READ SUCCEEDED
	kprintf("BUF CONTENTS: \n");
	kprintf("###################\n");
	kprintf("%s", (char *) buf);
	kprintf("###################\n");

	//TODO: connect stdout/err to console to support printing in user prog

	return strlen(buf); //TODO: check this
}

int sys_write(uint32_t fd, void *buf, size_t nbytes) {
	kprintf("\nWRITING FILE...%d %s %d\n", fd, (char *) buf, nbytes);
	struct FD **fds = curproc->fds;
	if (!valid_fd(fd, *fds)) {
		return EBADF;
	} else if (fds[fd]->free == true) {
		return EBADF; /* file must not be not open */
	} else if (!fds[fd]->can_write) {
		return EBADF; /* file was not opened for writing */
	}

	struct iovec iov;
	iov.iov_kbase = buf;
	iov.iov_len = nbytes;

	struct vnode *v = fds[fd]->file->v;
	struct uio u;
	uio_kinit(&iov, &u, buf, nbytes, fds[fd]->file->offset, UIO_WRITE);

	VOP_WRITE(v, &u);

	// Advance the file offset
	// TODO THIS ASSUMES THAT BUFLEN BYTES ARE READ - NOT ALWAYS THE CASE
	fds[fd]->file->offset += strlen(buf); //TODO: check this

	// TODO ERROR CHECK

	return strlen(buf); // TODO: check this
}
