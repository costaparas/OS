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
	struct addrspace *as = kmalloc(sizeof(struct addrspace));

	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	as->stackp = 0;
	as->nregions = 0;
	as->region_list = NULL;

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
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/*
	 * Write this.
	 */
}

void as_deactivate(void) {
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
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
	/*
	 * Write this.
	 */

	(void) executable; /* TODO: possibly unneeded */

	/* allocate space for new region and set up its fields */
	struct region *new_region = kmalloc(sizeof(struct region));
	/* TODO: should probably check if kmalloc fails */

	new_region->vbase     = vaddr;
	new_region->npages    = memsize / PAGE_SIZE; /* TODO: check this (should be OK, memsize is in bytes) */
	new_region->readable  = readable;
	new_region->writeable = writeable;
	/* TODO: add + set new_region->executeable? */

	as->nregions++;

	/* append new_region to as->region_list */
	struct region *curr = as->region_list;
	if (curr == NULL) {
		as->region_list = new_region;
	} else { /* Find last region and point its next to the newly created region */
		while (curr->next != NULL) curr = curr->next;
		curr->next = new_region;
	}

	/* TODO: potentially allocate pages (or do it in vm_fault) */

	return ENOSYS; /* TODO: what should really be returned on error? */
}

int as_prepare_load(struct addrspace *as) {
	/*
	 * Write this.
	 */

	(void) as;
	return 0;
}

int as_complete_load(struct addrspace *as) {
	/*
	 * Write this.
	 */

	(void) as;
	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
	/*
	 * Write this.
	 */

	(void) as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}
