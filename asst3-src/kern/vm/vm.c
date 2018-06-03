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

ftable_entry fhead = 0; /* pntr to first free entry in frame table */
uint32_t total_pages = 0; /* total pages in the hpt */
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;
struct lock *hpt_lock;

/*
 * Initialise the VM system by placing the frametable and page table just after the kernel.
 */
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
			/* last frame */
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

	/* update fhead to point to first free entry - i.e. after the ptable */
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
static void zero_region(vaddr_t paddr, unsigned npages) {
	KASSERT(paddr != 0 && npages != 0);
	bzero((void *)paddr, npages * PAGE_SIZE);
}

/*
 * Insert an entry into the ptable for the given vaddr.
 * Call alloc_kpages() to acquire a frame for the page.
 * Set the dirty bit as needed for write permissions.
 */
int insert_ptable_entry(struct addrspace *as, vaddr_t vaddr, int readable, int writeable, bool write_tlb) {
	KASSERT(as != NULL && vaddr != 0);
	(void) readable; /* unused */
	vaddr &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, vaddr);
	vaddr_t paddr = alloc_kpages(1);
	KASSERT(paddr % PAGE_SIZE == 0);
	if (paddr == 0) return ENOMEM; /* out of frames */

	bool release_lock = false;
	if (!lock_do_i_hold(hpt_lock)) {
		lock_acquire(hpt_lock);
		release_lock = true;
	}
	ptable_entry entry = &ptable[index];
	uint32_t i = index;
	bool overflow = false;

	/* do a linear scan until the 1st free slot is found */
	while (entry->entrylo & TLBLO_VALID) {
		overflow = true;
		i = (i + 1) % total_pages;
		if (i == index) {
			/* this is very unlikely, should run out of frames first */
			free_kpages(paddr);
			if (release_lock) lock_release(hpt_lock);
			return ENOMEM; /* out of pages */
		}
		entry = &ptable[i];
	}

	/* zero-fill the frame */
	zero_region(paddr, 1);

	ptable_entry curr = &ptable[index];

	/* if there was an overflow, find the end of the overflow chain */
	if (overflow) {
		while (curr->next != NULL) curr = curr->next;
		curr->next = entry; /* set the last entry in the chain to point to this new entry */
	}

	/* set pid and entryhi in the ptable entry */
	entry->pid = (uint32_t) as;
	entry->entryhi = vaddr;
	entry->next = NULL;

	/* set entrylo in the ptable entry */
	if (writeable) {
		entry->entrylo = KVADDR_TO_PADDR(paddr) | TLBLO_DIRTY | TLBLO_VALID;
	} else {
		entry->entrylo = KVADDR_TO_PADDR(paddr) | TLBLO_VALID;
	}

	/* write new ptable entry to tlb */
	if (!write_tlb) {
		if (release_lock) lock_release(hpt_lock);
		return 0;
	}
	int spl = splhigh();
	tlb_random(entry->entryhi, entry->entrylo);
	splx(spl);
	if (release_lock) lock_release(hpt_lock);
	return 0;
}

/*
 * Unset the dirty bit in the ptable entry for the given vaddr.
 */
void make_page_read_only(vaddr_t vaddr) {
	KASSERT(vaddr != 0);
	vaddr &= PAGE_FRAME;
	struct addrspace *as = proc_getas();
	lock_acquire(hpt_lock);

	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	ptable_entry curr = search_ptable(as, vaddr, NULL);

	/* unset dirty bit in entrylo */
	if (curr != NULL) {
		paddr_t paddr = curr->entrylo & TLBLO_PPAGE;
		curr->entrylo = paddr | TLBLO_VALID;
	}

	lock_release(hpt_lock);
}

/*
 * Remove page table entries and free frames associated with a region.
 */
void free_region(struct addrspace *as, vaddr_t vaddr, uint32_t npages) {
	lock_acquire(hpt_lock);
	for (vaddr_t page = vaddr; page != vaddr + npages * PAGE_SIZE; page += PAGE_SIZE) {
		ptable_entry prev = NULL;
		ptable_entry pt = search_ptable(as, page, prev); /* find ptable entry associated with page */
		if (pt == NULL) continue;
		KASSERT((pt->entryhi & TLBHI_VPAGE) == page);
		free_kpages(PADDR_TO_KVADDR(pt->entrylo & TLBLO_PPAGE));

		/* reset ptable entry */
		pt->pid = 0;
		pt->entryhi = 0;
		pt->entrylo = 0;

		if (prev != NULL) {
			/* update collision chain to "skip" this entry */
			prev->next = pt->next;
		} else if (pt->next != NULL) {
			/* head of chain - move last entry in chain to the start */
			ptable_entry curr = pt;
			prev = curr;

			/* get the last entry in the chain */
			while (curr->next != NULL) {
				prev = curr;
				curr = curr->next;
			}

			/* move entry to head of chain */
			pt->pid = curr->pid;
			pt->entryhi = curr->entryhi;
			pt->entrylo = curr->entrylo;
			curr->pid = 0;
			curr->entryhi = 0;
			curr->entrylo = 0;

			/* make 2nd-last slot of chain the last slot */
			prev->next = NULL;
		}
	}
	lock_release(hpt_lock);
}

/*
 * Copy ptable entries from old to new addrspace and allocate new frames.
 */
int copy_region(struct region *reg, struct addrspace *old, struct addrspace *newas) {
	vaddr_t addr = reg->vbase;
	lock_acquire(hpt_lock);
	while (addr != reg->vbase + reg->npages * PAGE_SIZE) {
		/* check an old page table entry exists for the page */
		ptable_entry old_pt = search_ptable(old, addr, NULL);
		if (old_pt != NULL) {
			/* insert page table entry for each page in the copied region */
			int ret = insert_ptable_entry(newas, addr, reg->readable, reg->writeable, false);
			if (ret) {
				lock_release(hpt_lock);
				return ret;
			}

			/* get ptable entry for new page */
			ptable_entry new_pt = search_ptable(newas, addr, NULL);
			if (new_pt == NULL) return ENOMEM; /* this should never happen, very unlikely */

			/* get frame number for old and new frames */
			vaddr_t old_frame = PADDR_TO_KVADDR(old_pt->entrylo & TLBLO_PPAGE);
			vaddr_t new_frame = PADDR_TO_KVADDR(new_pt->entrylo & TLBLO_PPAGE);

			/* copy the memory from the old frame to the new frame */
			memmove((void *) new_frame, (const void *) old_frame, PAGE_SIZE);
		}

		addr += PAGE_SIZE;
	}
	lock_release(hpt_lock);
	return 0;
}

/*
 * Find the ptable entry with the given vaddr and pid.
 * Begin the search from curr and follow the collision pointers until found.
 * Requires the page table lock to have been acquired already.
 * Also sets prev to point to the previous ptable entry in the collision chain.
 */
ptable_entry search_ptable(struct addrspace *as, vaddr_t vaddr, ptable_entry prev) {
	KASSERT(vaddr != 0);
	KASSERT((vaddr & PAGE_FRAME) == vaddr);
	KASSERT(lock_do_i_hold(hpt_lock));
	uint32_t index = hpt_hash(as, vaddr);
	pid_t pid = (uint32_t) as;
	ptable_entry curr = &ptable[index];
	do {
		if ((curr->entryhi & TLBHI_VPAGE) == vaddr && pid == curr->pid && (curr->entrylo & TLBLO_VALID)) break;
		prev = curr;
		curr = curr->next;
	} while (curr != NULL);
	(void) prev; /* make compiler happy, prev is a pntr meant to be used by caller */
	return curr;
}

/*
 * Return an index into the ptable for the given addrspace and addr.
 */
uint32_t hpt_hash(struct addrspace *as, vaddr_t addr) {
	KASSERT(as != NULL && addr != 0);
	KASSERT((addr & PAGE_FRAME) == addr);
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
		return EFAULT; /* attempt to write to read-only page */
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
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
		}
		curr_region = curr_region->next;
		nregions++;
	}
	if (region_found == NULL) return EFAULT;
	KASSERT(as->nregions == nregions);
	lock_acquire(hpt_lock);

	/* find ptable entry by traversing ptable using next pntrs to handle collisions */
	ptable_entry curr = search_ptable(as, faultaddress, NULL);

	if (curr == NULL) {
		lock_release(hpt_lock);

		/* lazy page/frame allocation */
		int ret = insert_ptable_entry(as, faultaddress, region_found->readable, region_found->writeable, true);
		if (ret) return ret;
	} else {
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
