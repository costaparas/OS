/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

extern int errno;

/* state relating to an open file on the system */
typedef struct OF {
	struct vnode *v; /* ptr to the underlying vnode */
	off_t offset; /* current index into the file, i.e. the file pointer */
} OF;

/* state relating to a file descriptor in a process */
typedef struct FD {
	bool free; /* used to mark this fd as available */
	OF *file; /* ptr to the open file entry */
	bool can_write; /* used to handle O_RDONLY open mode */
} FD;

OF *open_files = NULL; /* dynamically allocated open file table */

typedef struct fd_proc {
	struct FD fds[__OPEN_MAX]; /* per-process file descriptor table */
	void *proc; /* process this fd table belongs to */
} fd_proc;

fd_proc *fd_tables = NULL; /* dynamically allocated array of fd tables */

uint32_t num_files; /* number of open files on the system */
uint32_t num_proc; /* number of processes with at least one open fd, i.e. the
number of fd tables */

bool valid_fd(uint32_t, struct FD *fds);
struct FD *get_fd_table(void);
void fs_bootstrap(void);
void fs_clear_tables(void);

#endif /* _FILE_H_ */
