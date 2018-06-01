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
	as->nregions = 0;
	as->region_list = NULL;
	kprintf("addrspace ready\n");
	return as;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
	struct addrspace *newas = as_create();
	if (newas == NULL) return ENOMEM;

	/*********************************/
	/* TODO: implement as_copy fully */
	/*********************************/

	newas->nregions = old->nregions;
	newas->region_list = old->region_list;

	*ret = newas;
	return 0;
}

void as_destroy(struct addrspace *as) {
	/************************************/
	/* TODO: implement as_destroy fully */
	/************************************/

	kprintf("Running as_destroy, %d regions\n", as->nregions);
	/* Iterate through as->region_list and free each region */
	struct region *curr = as->region_list;

	int i = 0;
	while (curr != NULL) {
		/* TODO: check freeing stack in remove_ptable_entry() */
		kprintf("REMOVE REGION %d: %p, vbase: %d\n", i++, curr, curr->vbase & PAGE_FRAME);
		int result = remove_ptable_entry(as, curr->vbase & PAGE_FRAME);
		if (result) kprintf("Could not find page table entry to remove!\n");

		struct region *to_free = curr;
		curr = curr->next;
		kfree(to_free);
	}

	/* as_activate(); */

	/* TODO free frames for the as stack */

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
	new_region->can_write = writeable;

	as->nregions++;

	/* append new_region to as->region_list */
	struct region *curr_region = as->region_list;
	kprintf("NEW_REGION: %p\n", new_region);
	if (curr_region == NULL) {
		as->region_list = new_region;
	} else {
		struct region *old_head = as->region_list;
		new_region->next = old_head;
		as->region_list = new_region;
	}

	kprintf("new region created\n");
	return 0;
}

int as_prepare_load(struct addrspace *as) {
	kprintf("as_prepare_load, creating stack\n");

	/*
	 * initial stack pointer will be USERSTACK, see as_define_stack()
	 * base of user stack will be NUM_STACK_PAGES (16 pages) below this
	 * this needs to be set before the possiblity of vm_fault() being triggered
	 * vm_fault() maybe triggered before as_define_stack() is called
	 * but not before as_prepare_load() is called
	 */
	as_define_region(as, USERSTACK - PAGE_SIZE * NUM_STACK_PAGES, PAGE_SIZE * NUM_STACK_PAGES, true, true, false);

	/* set writable flag to true for all regions temporarily */
	for (struct region *curr = as->region_list; curr != NULL; curr = curr->next) {
		curr->writeable = true;
	}

	kprintf("as_prepare_load, stack created\n");
	return 0;
}

int as_complete_load(struct addrspace *as) {
	kprintf("starting as_complete_load\n");
	for (struct region *curr = as->region_list; curr != NULL; curr = curr->next) {
		/* reset write flag to real write flag */
		curr->writeable = curr->can_write;

		/* flip the dirty bit in the ptable entry for all entires for in this region */
		if (!curr->can_write) {
			for (vaddr_t addr = curr->vbase; addr != curr->vbase + curr->npages * PAGE_SIZE; addr += PAGE_SIZE) {
				make_page_read_only(addr);
			}
		}
	}

	as_activate(); /* flush the tlb since some entries are not read-only */
	kprintf("as_complete_load\n");
	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
	/* initial user-level stack pointer */
	kprintf("called as_define_stack\n");
	(void) as;
	*stackptr = USERSTACK;
	return 0;
}
