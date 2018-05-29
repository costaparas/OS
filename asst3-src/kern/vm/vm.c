#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <proc.h>
#include <current.h>
#include <spl.h>
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
		/* TODO: check this actually makes sense */
		(ptable + i)->pid = 0;
		(ptable + i)->entryhi = 0;
		(ptable + i)->entrylo = 0;
		(ptable + i)->next = NULL;
	}
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr) {
	uint32_t index;
	index = (((uint32_t) as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
	return index;
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
	/* TODO: handle these case later on */
	switch (faulttype) {
	case VM_FAULT_READONLY:
		panic("dumbvm: got VM_FAULT_READONLY\n");
		/* TODO: this should probably just return EFAULT! */
	case VM_FAULT_READ:
		/* TODO: need to check that vaddr is in region that is readable */
	case VM_FAULT_WRITE:
		/* TODO: need to check that vaddr is in region that is writeable */
		break;
	default:
		return EINVAL;
	}

	struct addrspace *as = proc_getas();
	if (curproc == NULL) return EFAULT;
	if (as == NULL) return EFAULT;

	/* TODO: addressspace error checking - i.e. check if vaddr is in a defined region */

	/* TODO: concurrency - HPT is global */

	pid_t pid = (uint32_t) as;
	faultaddress &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, faultaddress);
	if (((ptable + index)->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == (ptable + index)->pid) { /* TODO: may not check pid here */
		/* TODO: check if entry is valid ? */
		int spl = splhigh();
		tlb_random((ptable + index)->entryhi, (ptable + index)->entrylo);
		splx(spl);
		return 0;
	} else {
		ptable_entry curr = &ptable[index];
		while (curr->next != NULL) {
			curr = curr->next;
			if ((curr->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == curr->pid) break; /* TODO: see above */
		}
		if (curr == NULL) {
			return EFAULT;
		} else {
			/* TODO: check if entry is valid ? */
			int spl = splhigh();
			tlb_random(curr->entryhi, curr->entrylo);
			splx(spl);
		}
		return 0;
	}

	return EFAULT;
}

/*
 * SMP-specific functions. Unused in our configuration.
 */
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void) ts;
	panic("vm tried to do tlb shootdown?!\n");
}
