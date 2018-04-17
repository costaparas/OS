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

/*
 * Add your file-related functions here ...
 */

void fs_init() {
	for (int i = 0; i < NUM_FILES; i++) {
		fds[i].free = true;
		fds[i].offset = 0;
	}
}


int sys_open(void *path, uint32_t flags) {
	kprintf("open\n");

	int fd_found = -1;
	for (int curr = 0; curr < NUM_FILES; curr++) {
		if (fds[curr].free) {
			fd_found = curr; break;
		}
	}

	if (fd_found == -1) {
		// TODO ERROR HANDLING
		return -1;
	}


	struct vnode *v;
//	int result;

	/* Open the file. */
	//result =
	vfs_open(path, flags, 0, &v);

	fds[fd_found].v = v;

	return fd_found;
}

int sys_close(uint32_t fd) {
	kprintf("close\n");
//	int result; // TODO CHECK VFS CLOSE
	struct vnode *v = fds[fd].v;
	//result =
	vfs_close(v);
	return 0;
}

int sys_read(uint32_t fd, void *buf, size_t buflen) {
	kprintf("read\n");

	struct iovec iov;
	iov.iov_kbase = buf;
	iov.iov_len = buflen;

	struct vnode *v = fds[fd].v;
	struct uio u;
	uio_kinit(&iov, &u, buf, buflen, fds[fd].offset, UIO_READ);

	VOP_READ(v, &u);

	// TODO ERROR CHECK

	// TODO REMOVE THIS JUST SHOWS READ SUCCEEDED
	kprintf("BUF CONTENTS: \n");
	kprintf("###################\n");
	kprintf("%s\n", (char *) buf);
	kprintf("###################\n");
	return 0;
}
