#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

ftable_entry fhead;
struct frame_table_entry *ftable;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/*
 * Return a kernel virtual address, not a physical
 * address for some newly-allocated frame.
 * Only one frame can be allocated at a time.
 */
vaddr_t alloc_kpages(unsigned int npages) {
	if (npages != 1) return 0;
	paddr_t addr;
	spinlock_acquire(&stealmem_lock);
	if (ftable == 0) {
		/* use ram_stealmem if ftable isn't initialised */
		addr = ram_stealmem(npages);
	} else {
		if (fhead == NULL) {
			spinlock_release(&stealmem_lock);
			return 0; /* out of frames */
		}

		/* fhead->addr is stored as 20 bits so we need to shift it to form a paddr_t */
		addr = (paddr_t)(fhead->addr << PAGE_BITS);
		fhead = fhead->next;
	}
	spinlock_release(&stealmem_lock);
	if (addr == 0) return 0;
	return PADDR_TO_KVADDR(addr);
}

/*
 * Free the page at addr and set it to be the new head, pointing next to the old head.
 * The addr must be a kernel virtual address, not a physical address.
 */
void free_kpages(vaddr_t addr) {
	spinlock_acquire(&stealmem_lock);
	ftable_entry old_head = fhead;
	fhead = (ftable_entry) (KVADDR_TO_PADDR(addr) / PAGE_SIZE + ftable);
	fhead->next = old_head;
	spinlock_release(&stealmem_lock);
}
