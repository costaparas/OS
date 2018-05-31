/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

struct addrspace *as_create(void) {
	kprintf("init addrspace\n");
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) return NULL;

	/*
	 * Initialize as needed.
	 */
	as->stackp = 0;
	as->nregions = 0;
	as->region_list = NULL;
	kprintf("addrspace ready\n");
	return as;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
	struct addrspace *newas;

	newas = as_create();
	if (newas == NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */

	newas->stackp = old->stackp;
	newas->nregions = old->nregions;
	newas->region_list = old->region_list; /* TODO: does this suffice? */

	*ret = newas;
	return 0;
}

void as_destroy(struct addrspace *as) {
	/*
	 * Clean up as needed.
	 */

	/* Iterate through as->region_list and free each region */
	struct region *curr = as->region_list;
	while (curr != NULL) {
		/* TODO free all associated frames and other cleanup */

		struct region *to_free = curr;
		curr = curr->next;
		kfree(to_free);
	}

	kfree(as);
}

void as_activate(void) {
	struct addrspace *as = proc_getas();
	if (as == NULL) return;

	/* disable interrupts on this cpu while frobbing the tlb */
	int spl = splhigh();

	for (int i = 0; i < NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	splx(spl);
}

void as_deactivate(void) {
	as_activate(); /* TODO: check if this breaks anything */
}

/* zero-fill a region of memory */
static void as_zero_region(vaddr_t vaddr, unsigned npages) {
	(void)vaddr; (void)npages;
	//bzero((void *)(vaddr), npages * PAGE_SIZE); /* TODO: zero-fill memory */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
int readable, int writeable, int executable) {
	(void) executable; /* unused */

	/* allocate space for new region and set up its fields */
	struct region *new_region = kmalloc(sizeof(struct region));
	kprintf("about to create a new region\n");
	if (!new_region) return ENOMEM;

	size_t npages;

	/* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = memsize / PAGE_SIZE;
	KASSERT(npages != 0);

	new_region->vbase     = vaddr;
	new_region->npages    = npages;
	new_region->readable  = readable;
	new_region->writeable = writeable;

	as->nregions++;

	/* append new_region to as->region_list */
	struct region *curr_region = as->region_list;
	if (curr_region == NULL) {
		as->region_list = new_region;
	} else {
		struct region *old_head = as->region_list;
		new_region->next = old_head;
		as->region_list = new_region;
	}

	/* insert into page table (TODO: move to vm_fault later) */
	vaddr_t curr = vaddr;
	kprintf("allocating frames for the region\n");
	kprintf("npages in region: %u\n", new_region->npages);
	while (curr != vaddr + memsize) {
		int res = insert_ptable_entry(as, curr, readable, writeable);
		if (res) return res;
		curr += PAGE_SIZE;
	}
	as_zero_region(vaddr, npages);
	kprintf("new region created\n");
	return 0;
}

int as_prepare_load(struct addrspace *as) {
	/*
	 * Write this.
	 */

	kprintf("as_prepare_load, creating stack\n");

	/* initial stack pointer will be USERSTACK, see as_define_stack() */
	/* base of user stack will be NUM_STACK_PAGES (16 pages) below this */
	as->stackp = USERSTACK - PAGE_SIZE * NUM_STACK_PAGES;

	/* insert into page table (TODO: move to vm_fault later) */
	vaddr_t curr = as->stackp;
	while (curr != USERSTACK) {
		int res = insert_ptable_entry(as, curr, true, true);
		if (res) return res;
		curr += PAGE_SIZE;
	}
	as_zero_region(as->stackp, NUM_STACK_PAGES);
	kprintf("as_prepare_load, stack created\n");
	return 0;
}

int as_complete_load(struct addrspace *as) {
	/*
	 * Write this.
	 */

	(void) as;
	kprintf("as_complete_load\n");
	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
	/* initial user-level stack pointer */
	kprintf("called as_define_stack\n");
	KASSERT(as->stackp != 0);
	*stackptr = USERSTACK;
	return 0;
}
