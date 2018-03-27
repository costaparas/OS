/*
 * Copyright (C) 2017 Costa Paraskevopoulos.
 * Copyright (C) 2017 Dominic Fung.
 * Uses the quicksort algorithm to sort an array of integers.
 * Derived from https://en.wikipedia.org/wiki/Quicksort.
 */

#include "quicksort.h"

static void partition(unsigned int a[], int l, int r, int *p);
static void swap(unsigned int a[], int i, int j);

//sorts an array between the indexes l and r
void quicksort(unsigned int a[], int l, int r) {
	if (l < r) {
		int p;
		partition(a, l, r, &p);
		quicksort(a, l, p - 1);
		quicksort(a, p + 1, r);
	}
}

//partitions an array around a pivot value
//returns the index of the pivot value
static void partition(unsigned int a[], int l, int r, int *p) {
	unsigned int pivot = a[r]; //rightmost is pivot
	int i = l - 1;

	//arrange a to [(<=pivot)(pivot)(=>pivot)]
	int j = l;
	while (j != r) {
		if (a[j] <= pivot) {
			i = i + 1;
			swap(a, i, j);
		}
		j = j + 1;
	}
	swap(a, i + 1, r); //move pivot to the "middle"

	*p = i + 1;
}

//swaps two elements of an array at the given indexes
static void swap(unsigned int a[], int i, int j) {
	unsigned int temp = a[i];
	a[i] = a[j];
	a[j] = temp;
}
