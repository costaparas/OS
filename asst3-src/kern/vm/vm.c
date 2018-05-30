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

ftable_entry fhead = 0;
uint32_t hpt_size = 0;
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr);

void vm_bootstrap(void) {
	paddr_t size = ram_getsize(); /* must be called first, since ram_getfirstfree() invalidates it */
	vaddr_t start = PADDR_TO_KVADDR(ram_getfirstfree()); /* top of kernel */

	/* place frame table right after kernel */
	ftable = (ftable_entry) start;

	/* set first free frame to be the frame immediately after kernel */
	uint32_t num_frames = KVADDR_TO_PADDR(start) / PAGE_SIZE; /* frames used by kernel */
	fhead = &ftable[num_frames];

	/* init frame table */
	for (uint32_t i = 0; i < size / PAGE_SIZE; ++i) {
		ftable[i].addr = i;
		if (i == size / PAGE_SIZE - 1) {
			ftable[i].next = NULL; /* TODO: out of memory, not a circular linked list */
		} else {
			ftable[i].next = &ftable[i + 1];
		}
	}

	/* update fhead to point to first free entry - i.e. after the ftable */
	fhead += ((size / PAGE_SIZE) * sizeof(struct frame_table_entry)) / PAGE_SIZE;

	/* place page table right after frame table */
	ptable = (ptable_entry) (start + ((size / PAGE_SIZE) * sizeof(struct frame_table_entry)));

	/* update fhead to point to first free entry - i.e. after tge ptable */
	hpt_size = (size / PAGE_SIZE) * 2; /* size hpt to twice as many physical frames */
	fhead += ((hpt_size * sizeof(struct page_table_entry)) / PAGE_SIZE);

	/* init page table */
	for (uint32_t i = 0; i < hpt_size; ++i) {
		/* TODO: check this actually works for initialisation */
		ptable[i].pid = 0;
		ptable[i].entryhi = 0;
		ptable[i].entrylo = 0;
		ptable[i].next = NULL;
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
	ptable_entry curr = &ptable[index];

	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	do {
		if ((curr->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == curr->pid) break; /* TODO: may not need to check pid? */
		curr = curr->next;
	} while (curr->next != NULL);

	if (curr == NULL) {
		return EFAULT;
	} else {
		/* TODO: check if entry is valid ? */
		int spl = splhigh();
		tlb_random(curr->entryhi, curr->entrylo);
		splx(spl);
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
