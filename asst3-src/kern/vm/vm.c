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
#include <synch.h>

/* Place your page table functions here */

ftable_entry fhead = 0;
uint32_t total_pages = 0; /* total pages in the hpt */
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

struct lock *hpt_lock;

void vm_bootstrap(void) {
	/* create hpt lock */
	hpt_lock = lock_create("hpt_lock");
	KASSERT(hpt_lock != NULL);

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
		ptable[i].pid = 0;
		ptable[i].entryhi = 0;
		ptable[i].entrylo = 0;
		ptable[i].next = NULL;
	}
}

/*
 * Zero-fill a region of memory. The paddr must be a kernel virtual address.
 */
static void zero_region(paddr_t paddr, unsigned npages) {
	KASSERT(paddr != 0 && npages != 0);
	bzero((void *)paddr, npages * PAGE_SIZE);
}

/*
 * Insert an entry into the ptable for the given vaddr.
 * Call alloc_kpages() to acquire a frame for the page.
 * Set the dirty bit as needed for write permissions.
 */
int insert_ptable_entry(struct addrspace *as, vaddr_t vaddr, int readable, int writeable) {
	KASSERT(as != NULL && vaddr != 0);
	(void) readable; /* TODO: is this needed? */
	vaddr &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, vaddr);
	vaddr_t paddr = alloc_kpages(1);
	if (paddr == 0) return ENOMEM; /* out of frames */

	lock_acquire(hpt_lock);
	ptable_entry entry = &ptable[index];
	uint32_t i = index;
	bool overflow = false;

	/* do a linear scan until the 1st free slot is found */
	while (entry->entrylo & TLBLO_VALID) {
		overflow = true;
		i = (i + 1) % total_pages;
		if (i == index) {
			lock_release(hpt_lock);
			return ENOMEM; /* out of pages */
		}
		entry = &ptable[i];
	}

	/* zero-fill the frame */
	zero_region(paddr, 1);

	ptable_entry curr = &ptable[index];

	/* if there was an overflow, find the end of the overflow chain */
	if (overflow) {
		while (overflow == true && curr->next != NULL) curr = curr->next;
		curr->next = entry; /* set the last entry in the chain to point to this new entry */
	}

	/* set pid and entryhi in the ptable entry */
	entry->pid = (uint32_t) as;
	entry->entryhi = vaddr;

	/* set entrylo in the ptable entry */
	if (writeable) {
		entry->entrylo = KVADDR_TO_PADDR(paddr) | TLBLO_DIRTY | TLBLO_VALID;
	} else {
		entry->entrylo = KVADDR_TO_PADDR(paddr) | TLBLO_VALID;
	}
	kprintf("PADDRT: %d\n", paddr);
	lock_release(hpt_lock);

	/* write new ptable entry to tlb */
	int spl = splhigh();
	tlb_random(entry->entryhi, entry->entrylo);
	splx(spl);
	return 0;
}

/*
 * Unset the dirty bit in the ptable entry for the given vaddr.
 */
void make_page_read_only(vaddr_t vaddr) {
	KASSERT(vaddr != 0);
	vaddr &= PAGE_FRAME;
	struct addrspace *as = proc_getas();
	pid_t pid = (uint32_t) as;
	uint32_t index = hpt_hash(as, vaddr);
	ptable_entry curr = &ptable[index];

	lock_acquire(hpt_lock);
	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	curr = search_ptable(curr, vaddr, pid);

	/* unset dirty bit in entrylo */
	if (curr != NULL) {
		paddr_t paddr = curr->entrylo & TLBLO_PPAGE;
		curr->entrylo = paddr | TLBLO_VALID;
	}

	lock_release(hpt_lock);
}

/*
 * Removes a page table entry and frees the frame associated with it
 */
uint32_t remove_ptable_entry(struct addrspace *as, vaddr_t vaddr) {
	uint32_t index = hpt_hash(as, vaddr);

	lock_acquire(hpt_lock);
	ptable_entry pt = &ptable[index];

	pt = search_ptable(pt, vaddr, (pid_t) as);
	if (pt == NULL) panic("Error in as_destroy(): page table not found\n");
	KASSERT((pt->entryhi & TLBHI_VPAGE) == vaddr);

	vaddr_t frame = PADDR_TO_KVADDR(pt->entrylo & PAGE_FRAME);
	kprintf("FREEING FRAME: %d\n", frame);
	free_kpages(frame);

	/* Clear out pt's fields */
	pt->pid = 0;
	pt->entryhi = 0;
	pt->entrylo = 0;

	lock_release(hpt_lock);
	return 0;
}

/*
 * Find the ptable entry with the given vaddr and pid.
 * Begin the search from curr and follow the collision pointers until found.
 */
ptable_entry search_ptable(ptable_entry curr, vaddr_t vaddr, pid_t pid) {
	KASSERT(curr != NULL && vaddr != 0);
	do {
		if ((curr->entryhi & TLBHI_VPAGE) == vaddr && pid == curr->pid) break; /* TODO: may not need to check pid? */
		curr = curr->next;
	} while (curr != NULL);
	return curr;
}

/*
 * Return an index into the ptable for the given addrspace and addr.
 */
uint32_t hpt_hash(struct addrspace *as, vaddr_t addr) {
	KASSERT(as != NULL && addr != 0);
	uint32_t index;
	index = (((uint32_t) as) ^ (addr >> PAGE_BITS)) % total_pages;
	return index;
}

/*
 * Find the ptable entry corresponding to the faultaddress and load into the tlb.
 */
int vm_fault(int faulttype, vaddr_t faultaddress) {
	switch (faulttype) {
	case VM_FAULT_READONLY:
		panic("writing to read-only page\n"); /* TODO: debug-only */
		return EFAULT; /* attempt to write to read-only page */
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break; /* these cases are handled below */
	default:
		return EINVAL; /* unknown faulttype */
	}
	if (curproc == NULL) return EFAULT;
	struct addrspace *as = proc_getas();
	if (as == NULL) return EFAULT;

	/* assert that the address space has been set up properly */
	KASSERT(as->region_list != NULL);
	struct region *curr_region = as->region_list;
	uint32_t nregions = 0;
	struct region *region_found = NULL;
	faultaddress &= PAGE_FRAME;

	while (curr_region != NULL) {
		/* assert that region is set up correctly */
		KASSERT(curr_region->vbase != 0);
		KASSERT(curr_region->npages != 0);
		KASSERT((curr_region->vbase & PAGE_FRAME) == curr_region->vbase);

		/* check if vaddr is in a valid region */
		if (!region_found && faultaddress >= curr_region->vbase && faultaddress < curr_region->vbase + curr_region->npages * PAGE_SIZE) {
			region_found = curr_region;
			/* TODO: i don't think these are needed at all - these are not meant to be enforced */
			/* TODO: only the VM_FAULT_READONLY case needs to be handled */
			//if (VM_FAULT_READ && !curr_region->readable) {
			//	panic("reading from a non-readable region %u\n", faultaddress); /* TODO: debug-only */
			//	return EFAULT;
			//}
			//if (VM_FAULT_WRITE && !curr_region->writeable) {
			//	panic("writing to a non-writable region %u\n", faultaddress); /* TODO: debug-only */
			//	return EFAULT;
			//}
		}
		curr_region = curr_region->next;
		nregions++;
	}
	if (region_found == NULL) {
		panic("not in a region\n"); /* TODO: debug-only */
		return EFAULT;
	}
	KASSERT(as->nregions == nregions);

	pid_t pid = (uint32_t) as;
	uint32_t index = hpt_hash(as, faultaddress);
	ptable_entry curr = &ptable[index];

	lock_acquire(hpt_lock);

	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	curr = search_ptable(curr, faultaddress, pid);

	if (curr == NULL) {
		lock_release(hpt_lock);
		/* lazy page/frame allocation */
		int ret = insert_ptable_entry(as, faultaddress, region_found->readable, region_found->writeable);
		if (ret) return ret;
	} else {
		/* TODO: check if entry is valid? */
		int spl = splhigh();
		tlb_random(curr->entryhi, curr->entrylo);
		splx(spl);
		lock_release(hpt_lock);
	}
	return 0;
}

/*
 * SMP-specific functions. Unused in our configuration.
 */
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void) ts;
	panic("vm tried to do tlb shootdown?!\n");
}
