#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_bootstrap(void)
{
	/* Initialise VM sub-system.  You probably want to initialise your 
	   frame table here as well.
	*/

	vaddr_t head = PADDR_TO_KVADDR(ram_getfirstfree());
	ftable = (struct frame_table_entry *) head;
	paddr_t size = ram_getsize();
	for (uint32_t i = 0; i < size; ++i) {
		(ftable + i)->addr = i >> 12;
		if (i == size + 1) {
			(ftable + i)->next = (i + 1) >> 12;
		} else {
			(ftable + i)->next = 0;
		}
	}

}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	(void) faulttype;
	(void) faultaddress;

	panic("vm_fault hasn't been written yet\n");

	return EFAULT;
}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

