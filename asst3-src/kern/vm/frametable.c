#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */

vaddr_t fhead;
struct frame_table_entry *ftable;

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t alloc_kpages(unsigned int npages) {
	if (npages != 1) return 0;

	paddr_t addr;

	spinlock_acquire(&stealmem_lock);
	if (ftable == 0) {
		addr = ram_stealmem(npages);
	} else {
		addr = fhead;
		fhead = (vaddr_t)(((struct frame_table_entry *) fhead)->next);
	}
	spinlock_release(&stealmem_lock);

	if (addr == 0) return 0;

	return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr) {
	spinlock_acquire(&stealmem_lock);
	vaddr_t old_head = fhead;
	fhead = KVADDR_TO_PADDR(addr);
	((struct frame_table_entry *) fhead)->next = old_head;
	spinlock_release(&stealmem_lock);
}
