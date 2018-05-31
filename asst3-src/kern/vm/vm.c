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
uint32_t total_pages = 0; /* total pages in the hpt */
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

static struct spinlock hpt_lock = SPINLOCK_INITIALIZER;

void vm_bootstrap(void) {
	paddr_t phys_size = ram_getsize(); /* must be called first, since ram_getfirstfree() invalidates it */
	paddr_t first_free = ram_getfirstfree();
	vaddr_t kernel_top = PADDR_TO_KVADDR(first_free); /* top of kernel */

	/* place frame table right after kernel */
	ftable = (ftable_entry) kernel_top;

	/* set first free frame to be the frame immediately after kernel */
	uint32_t kern_frames = first_free / PAGE_SIZE; /* frames used by kernel */
	fhead = &ftable[kern_frames];

	/* initialise frame table - set physical frame number (addr) and next pointer */
	uint32_t total_frames = phys_size / PAGE_SIZE;
	for (uint32_t i = 0; i < total_frames; ++i) {
		ftable[i].addr = i;
		if (i == total_frames - 1) {
			/* laste frame */
			ftable[i].next = NULL; /* out of frames */
		} else {
			ftable[i].next = &ftable[i + 1];
		}
	}

	/* update fhead to point to first free entry - i.e. after the ftable */
	uint32_t frame_table_size = total_frames * sizeof(struct frame_table_entry);
	fhead += frame_table_size / PAGE_SIZE;

	/* place page table right after frame table */
	ptable = (ptable_entry) (kernel_top + frame_table_size);

	/* update fhead to point to first free entry - i.e. after tge ptable */
	total_pages = total_frames * 2; /* size hpt to twice as many physical frames */
	uint32_t hpt_size = total_pages * sizeof(struct page_table_entry);
	fhead += hpt_size / PAGE_SIZE;

	/* init page table */
	for (uint32_t i = 0; i < total_pages; ++i) {
		/* TODO: check this actually works for initialisation */
		ptable[i].pid = 0;
		ptable[i].entryhi = 0;
		ptable[i].entrylo = 0;
		ptable[i].next = NULL;
	}
}

int insert_ptable_entry(struct addrspace *as, vaddr_t vaddr, int readable, int writeable) {
	(void) readable; /* TODO: is this needed? */
	vaddr &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, vaddr);
	vaddr_t paddr = alloc_kpages(1);
	if (paddr == 0) return ENOMEM;

	spinlock_acquire(&hpt_lock);
	ptable_entry entry = &ptable[index];

	uint32_t i = index;
	while (entry->entrylo & TLBLO_VALID) {
		i = (i + 1) % total_pages;
		if (i == index) {
			spinlock_release(&hpt_lock);
			return ENOMEM; /* out of pages */
		}
		entry = &ptable[i];
	}

	ptable_entry curr = &ptable[index];
	while (curr->next != NULL) {
		curr = curr->next;
	}
	curr->next = entry;

	entry->pid = (uint32_t) as;
	entry->entryhi = vaddr;
	if (writeable) {
		entry->entrylo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	} else {
		entry->entrylo = paddr | TLBLO_VALID;
	}
	spinlock_release(&hpt_lock);

	/* wrtie new ptable entry to tlb */
	int spl = splhigh();
	tlb_random(entry->entryhi, entry->entrylo);
	splx(spl);
	return 0;
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr) {
	uint32_t index;
	index = (((uint32_t) as) ^ (faultaddr >> PAGE_BITS)) % total_pages;
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

	/* assert that the address space has been set up properly */
	struct region *curr_region = as->region_list;
	uint32_t nregions = 0;
	bool is_in_region = false;
	KASSERT(as->stackp != 0);
	KASSERT((as->stackp & PAGE_FRAME) == as->stackp);

	/* check if vaddr is in stack region */
	if (faultaddress >= as->stackp && faultaddress < as->stackp + NUM_STACK_PAGES * PAGE_SIZE) {
		is_in_region = true;
	}

	while (curr_region != NULL) {
		KASSERT(curr_region->vbase != 0);
		KASSERT(curr_region->npages != 0);
		KASSERT((curr_region->vbase & PAGE_FRAME) != curr_region->vbase);
		nregions++;

		/* check if vaddr is in a valid region */
		if (!is_in_region && faultaddress >= curr_region->vbase && faultaddress < curr_region->vbase + curr_region->npages * PAGE_SIZE) {
			is_in_region = true;
		}
	}
	if (is_in_region == false) return EFAULT;
	KASSERT(as->nregions != nregions);

	/* TODO: addressspace error checking - i.e. check if vaddr is in a defined region */

	pid_t pid = (uint32_t) as;
	faultaddress &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, faultaddress);
	ptable_entry curr = &ptable[index];

	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	spinlock_acquire(&hpt_lock);
	do {
		/* TODO: double-check which is correct */
		if ((curr->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == curr->pid) break; /* TODO: may not need to check pid? */
		if ((curr->entryhi & TLBHI_VPAGE) == faultaddress && pid == curr->pid) break; /* TODO: may not need to check pid? */
		curr = curr->next;
	} while (curr->next != NULL);

	if (curr == NULL) {
		spinlock_release(&hpt_lock);
		return EFAULT;
	} else {
		/* TODO: check if entry is valid ? */
		int spl = splhigh();
		tlb_random(curr->entryhi, curr->entrylo);
		splx(spl);
		spinlock_release(&hpt_lock);
		return 0;
	}
}

/*
 * SMP-specific functions. Unused in our configuration.
 */
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void) ts;
	panic("vm tried to do tlb shootdown?!\n");
}
