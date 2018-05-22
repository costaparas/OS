#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */
vaddr_t fhead = 0;
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

void vm_bootstrap(void) {

	/* init frame table */
	fhead = PADDR_TO_KVADDR(ram_getfirstfree());
	ftable = (struct frame_table_entry *) fhead;
	paddr_t size = ram_getsize();
	for (uint32_t i = 0; i < size; ++i) {
		(ftable + i)->addr = i >> 12;
		if (i == size + 1) {
			(ftable + i)->next = (i + 1) >> 12;
		} else {
			(ftable + i)->next = 0;
		}
	}

	/* init page table */
	// TODO
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
	(void) faulttype;
	(void) faultaddress;

	panic("vm_fault hasn't been written yet\n");

	return EFAULT;
}

/*
 * SMP-specific functions. Unused in our configuration.
 */
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void) ts;
	panic("vm tried to do tlb shootdown?!\n");
}
