#include <types.h>
#include <lib.h>

#include "item.h"
#include "Queue.h"

typedef struct node *link;

typedef struct node {
	item data;
	link next;
} node;

//list-like concrete representation of queue
typedef struct queue {
	link head, tail;
	unsigned long length;
} queue;

//creates and returns a new empty queue
Queue create_queue(void) {
	Queue new = kmalloc(sizeof(queue));
	KASSERT(new != NULL);
	new->length = 0;
	new->head = new->tail = NULL;
	return new;
}

//frees all memory associated with a given queue
void dispose_queue(Queue q) {
	KASSERT(q != NULL);
	item temp;
	while (is_queue_empty(q) == false) {
		temp = dequeue(q);
		del(temp);
	}
	kfree(q);
}

//inserts a new item at the back of a given queue
void enqueue(Queue q, item data) {
	KASSERT(q != NULL);
	link new = kmalloc(sizeof(node));
	KASSERT(new != NULL);
	new->data = copy(data);
	new->next = NULL;
	if (is_queue_empty(q) == true) {
		q->head = new;
	} else {
		q->tail->next = new;
	}
	q->tail = new;
	q->length++;
}

//removes and returns a copy of the front item of a given queue
item dequeue(Queue q) {
	KASSERT(q != NULL);
	KASSERT(is_queue_empty(q) == false);
	link front = q->head;
	item data = copy(front->data);
	q->head = front->next;
	del(front->data);
	kfree(front);
	q->length--;
	return data;
}

//returns the data at the front of a given queue
item get_head(Queue q) {
	KASSERT(q != NULL);
	KASSERT(is_queue_empty(q) == false);
	return q->head->data;
}

//returns the length of a given queue
unsigned long get_queue_length(Queue q) {
	KASSERT(q != NULL);
	return q->length;
}

//indicates whether a given queue is empty
bool is_queue_empty(Queue q) {
	KASSERT(q != NULL);
	return q->length == 0;
}
