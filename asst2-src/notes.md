UPCOMING CONSIDERATIONS
####################
-COSTA: Attach stdout/stderr to console
-DOM: USE UIO_USERSPACE INSTEAD OF SYS - CHECK SAMPLE HELPER FUNCTION IN ASST2.PDF INSTEAD OF UIO_KINIT()
-lseek - offset is 64 bits - can't store in a0-a3, flags goes onto stack as well, sp + 16. CHECK OUT join32to64, split64to32
-vnode.h - will need to check vop_isseekable in lseek() - STDIN/STDOUT/STDERR (stream files) and dirs (ESPIPE) - "cache" isseekable in our tables
-write/lseek: might need to modify return code to populate v0 and v1 if returning 64 bit value
-write/read: (extra argument in syscall) handle returning byte sizes etc. - add extra int/whatev variable in syscall() to track and pass pointer to implementation
-Need to maintain reference count - other FDs might point to open file table
-Use spinlock in proc struct to handle concurrency
-Change KASSERT to return ERNOMEM
-In proc_end (WHEREVER THAT IS), kfree the fd_table
-Error handling - write up a flowchart with each of the different cases to handle
-Check assert for init_fd_table()

NOTES FROM TUTORIAL
#####
-Don't need vfs_lookup - normally used if checking a file already exists or not
-Make new uio for each read

DONE
####
-userptr_t - differentiate (originating from user-level) pointers from kernel-level - can't dereference without casting
-Use copyin/copyout if dealing with user pointers
-CAN'T JUST USE PATH DIRECTLY - COPY INTO KERNEL BUFFER
-struct proc: - ADD PER-PROCESS MATERIAL IN PROC.H
-Move krealloc to malloc.h/c
