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

int errno = 0;

/*
 * Initialize state required for the open file and file descriptor tables.
 */
void fs_bootstrap() {
	kprintf("\nINIT FS...\n");
	num_files = 0;
	num_proc = 0;
}

/*
 * Free memory used by the open file and file descriptor tables.
 */
void fs_clear_tables() {
	kprintf("\nFREEING FS...\n");
	kfree(fd_tables);
	kfree(open_files);
}

/*
 * Return the fd table for the current process; NULL if non-existent.
 */
struct FD *get_fd_table(void) {
	for (uint32_t i = 0; i < num_proc; ++i) {
		if (fd_tables[i].proc == curproc) {
			return fd_tables[i].fds;
		}
	}
	return NULL;
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
 * Open a file in the specified mode. Return a process-unique file
 * descriptor on success; otherwise, return -1 and set errno appropriately.
 */
int sys_open(void *path, uint32_t flags) {
	kprintf("\nOPENING FILE...%s %d\n", (char *) path, flags);
	struct FD *fds = get_fd_table();

	/* if this is the first file opened by the process, create an fd table */
	if (fds == NULL) {
		struct fd_proc *tmp = kmalloc(sizeof(fd_proc) * (num_proc + 1));
		KASSERT(tmp != NULL);
		memcpy(tmp, fd_tables, sizeof(fd_proc) * num_proc);
		kfree(fd_tables);
		fd_tables = tmp;
		fds = fd_tables[num_proc].fds;
		fd_tables[num_proc++].proc = curproc;
	}

	/* look for an available fd */
	int fd_found = -1;
	for (int i = 0; i < __OPEN_MAX; i++) {
		if (fds[i].free) {
			fd_found = i;
			break;
		}
	}
	if (fd_found == -1) {
		errno = ENFILE; /* reached max open files for this process */
		return -1;
	}

	/* TODO: implement more error handling before calling vfs_open */

	/* open the file - will increase the ref count in vnode */
	struct vnode *v;
	int result = vfs_open(path, flags, 0, &v);
	if (result != 0) {
		errno = result;
		return -1;
	}

	/* look for vnode in open file table */
	fds[fd_found].file = NULL;
	for (uint32_t i = 0; i < num_files; ++i) {
		if (open_files[i].v == v) {
			fds[fd_found].file = &open_files[i];
			break;
		}
	}

	/* otherwise, create a new entry in open file table for the vnode */
	if (fds[fd_found].file == NULL) {
		struct OF *tmp = kmalloc(sizeof(OF) * (num_files + 1));
		KASSERT(tmp != NULL);
		memcpy(tmp, open_files, sizeof(OF) * num_files);
		kfree(open_files);
		open_files = tmp;
		open_files[num_files].offset = 0;
		open_files[num_files].v = v;
		fds[fd_found].file = &open_files[num_files++];
	}

	fds[fd_found].free = false; /* mark this fd as used */
	return fd_found;
}

/*
 * Close an open file. Return -1 on error and set errno appropriately.
 */
int sys_close(uint32_t fd) {
	kprintf("\nCLOSING FILE...%d\n", fd);
	struct FD *fds = get_fd_table();
	if (!valid_fd(fd, fds)) {
		errno = EBADF;
		return -1;
	}
	if (fds[fd].free == false) {
		vfs_close((fds[fd].file)->v); /* hard i/o error is unlikely and
		rarely checked - see kern/vfs/vfspath.c */
		fds[fd].free = true; /* fd can be re-used for this process */
		return 0;
	}
	errno = EBADF; /* fd must not be open */
	return -1;
}

int sys_read(uint32_t fd, void *buf, size_t buflen) {
	kprintf("\nREADING FILE...%d %s %d\n", fd, (char *) buf, buflen);
	struct FD *fds = get_fd_table();
	if (!valid_fd(fd, fds)) {
		errno = EBADF;
		return -1;
	}
	if (fds[fd].free == true) {
		errno = EBADF; /* file must not be not open */
		return -1;
	}

	struct iovec iov;
	iov.iov_kbase = buf;
	iov.iov_len = buflen;

	struct vnode *v = fds[fd].file->v;
	struct uio u;
	uio_kinit(&iov, &u, buf, buflen, fds[fd].file->offset, UIO_READ);

	VOP_READ(v, &u);

	// Advance the file offset
	// TODO THIS ASSUMES THAT BUFLEN BYTES ARE READ - NOT ALWAYS THE CASE
	fds[fd].file->offset += buflen;

	// TODO ERROR CHECK

	// TODO REMOVE THIS JUST SHOWS READ SUCCEEDED
	kprintf("BUF CONTENTS: \n");
	kprintf("###################\n");
	kprintf("%s", (char *) buf);
	kprintf("###################\n");

	return 0;
}
