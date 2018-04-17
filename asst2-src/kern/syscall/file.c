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

int sys_open(void *path, uint32_t flags) {
	kprintf("open\n");
	struct vnode *v;
//	int result;

	/* Open the file. */
	//result =
	vfs_open(path, flags, 0, &v);

	fds[open_files++].v = v;
	return open_files-1;
}

int sys_close(uint32_t fd) {
	kprintf("close\n");
//	int result;
	struct vnode *v = fds[fd].v;
	//result =
	vfs_close(v);
	return 0;
}
