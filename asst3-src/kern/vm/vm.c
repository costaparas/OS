#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */
vaddr_t fhead = 0;
uint32_t hpt_size = 0;
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr);

void vm_bootstrap(void) {

	/* init frame table */
	fhead = PADDR_TO_KVADDR(ram_getfirstfree()); /* top of kernel */
	ftable = (struct frame_table_entry *) fhead;
	paddr_t size = ram_getsize();
	for (uint32_t i = 0; i < size; ++i) {
		(ftable + i)->addr = i >> PAGE_BITS;
		if (i == size - 1) {
			(ftable + i)->next = 0; /* circular linked list */
		} else {
			(ftable + i)->next = (i + 1) >> PAGE_BITS;
		}
	}

	/* set first free frame to be the frame immediately after ftable */
	fhead += size;

	/* init page table */
	hpt_size = size * 2;
	ptable = (struct page_table_entry *) fhead;
	fhead += hpt_size;
	for (uint32_t i = 0; i < hpt_size; ++i) {
		/* only need to ensure all next ptrs are null */
		(ptable + i)->next = NULL;
	}
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr) {
	uint32_t index;
	index = (((uint32_t) as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
	return index;
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
