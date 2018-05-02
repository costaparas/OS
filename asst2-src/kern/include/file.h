/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

/* state relating to an open file on the system */
typedef struct OF {
	struct vnode *v; /* ptr to the underlying vnode */
	off_t offset; /* current index into the file, i.e. the file pointer */
} OF;

/* state relating to a file descriptor in a process */
typedef struct FD {
	bool free; /* used to mark this fd as available */
	OF *file; /* ptr to the open file entry */
	bool can_read; /* used to handle O_RDONLY open mode */
	bool can_write; /* used to handle O_WRONLY open mode */
} FD;

extern struct OF **open_files; /* dynamically allocated open file table */
uint32_t num_files; /* number of open files on the system */

/* function prototypes for helpers */
bool valid_fd(uint32_t fd);
void fs_bootstrap(void);
void fs_clear_tables(void);
int init_fd_table(void);

#endif /* _FILE_H_ */
