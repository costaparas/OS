UPCOMING CONSIDERATIONS
=======================
read
----
* More error handling and checking (comes under flowchart below)

write
-----
* More error handling and checking (comes under flowchart below)

lseek
-----
* lseek - offset is 64 bits - can't store in a0-a3, flags goes onto stack as well, sp + 16. CHECK OUT join32to64, split64to32
* vnode.h - will need to check vop_isseekable in lseek() - STDIN/STDOUT/STDERR (stream files) and dirs (ESPIPE) - "cache" isseekable in our tables

Combinations
------------
* write/lseek: might need to modify return code to populate v0 and v1 if returning 64 bit value
* write/read: (extra argument in syscall) handle returning byte sizes etc. - add extra int/whatev variable in syscall() to track and pass pointer to implementation
* All syscalls: error handling - write up a flowchart with each of the different cases to handle
* VERY DETAILED TESTING

NOTE:
need to handle concurrency issues using locks - open_file table & num_files
since processes can be assumed to be single-threaded so ignore fd table
synchronization (may need to use spinlock in struct proc anyway though?)

FD/OF design
------------
* Need to maintain reference count - other FDs might point to open file table
* Use spinlock in proc struct to handle concurrent access

NOTES FROM TUTORIAL
===================
* Don't need vfs_lookup - normally used if checking a file already exists or not
* Make new uio for each read

DONE
====
* Don't reallocate new open file table on each new program run
* In proc_end (WHEREVER THAT IS), kfree the fd_table
* COSTA: Attach stdout/stderr to console and check that it works
* Check why random garbage data gets written as well 
* DOM: USE UIO_USERSPACE INSTEAD OF SYS - CHECK SAMPLE HELPER FUNCTION IN ASST2.PDF INSTEAD OF UIO_KINIT()
* userptr_t - differentiate (originating from user-level) pointers from kernel-level - can't dereference without casting
* Check assert for each individual kmalloc in init_fd_table()
* Use copyin/copyout if dealing with user pointers
* CAN'T JUST USE PATH DIRECTLY - COPY INTO KERNEL BUFFER
* struct proc: - ADD PER-PROCESS MATERIAL IN PROC.H
* Move krealloc to malloc.h/c
* Change KASSERT to return ERNOMEM
