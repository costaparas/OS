/*
 * Copyright (C) 2017 Costa Paraskevopoulos.
 * Copyright (C) 2017 Dominic Fung.
 * Uses the quicksort algorithm to sort an array of integers.
 * Derived from https://en.wikipedia.org/wiki/Quicksort.
 */

#include "quicksort.h"

static void partition(int a[], int l, int r, int N, int *p);
static void swap(int a[], int i, int j, int N);

//sorts an array between the indexes l and r
void quicksort(int a[], int l, int r, int N) {
	if (l < r) {
		int p;
		partition(a, l, r, N, &p);
		quicksort(a, l, p - 1, N);
		quicksort(a, p + 1, r, N);
	}
}

//partitions an array around a pivot value
//returns the index of the pivot value
static void partition(int a[], int l, int r, int N, int *p) {
	int pivot = a[r]; //rightmost is pivot
	int i = l - 1;

	//arrange a to [(<=pivot)(pivot)(=>pivot)]
	int j = l;
	while (j != r) {
		if (a[j] <= pivot) {
			i = i + 1;
			swap(a, i, j, N);
		}
		j = j + 1;
	}
	swap(a, i + 1, r, N); //move pivot to the "middle"

	*p = i + 1;
}

//swaps two elements of an array at the given indexes
static void swap(int a[], int i, int j, int N) {
	int temp = a[i];
	a[i] = a[j];
	a[j] = temp;
}
