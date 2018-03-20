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
Queue createQueue(void) {

   Queue new = malloc(sizeof(queue));
   assert(new != NULL);
   new->length = 0;
   new->head = new->tail = NULL;
   return new;
}

//frees all memory associated with a given queue
void disposeQueue(Queue q) {

   assert(q != NULL);
   item temp;
   while (isQueueEmpty(q) == FALSE) {
      temp = dequeue(q);
      del(temp);
   }
   free(q);
}

//inserts a new item at the back of a given queue
void enqueue(Queue q, item data) {

   assert(q != NULL);
   link new = malloc(sizeof(node));
   assert(new != NULL);
   new->data = copy(data);
   new->next = NULL;
   if (isQueueEmpty(q) == TRUE) {
      q->head = new;
   } else {
      q->tail->next = new;
   }
   q->tail = new;
   q->length++;
}

//removes and returns a copy of the front item of a given queue
item dequeue(Queue q) {

   assert(q != NULL);
   assert(isQueueEmpty(q) == FALSE);
   link front = q->head;
   item data = copy(front->data);
   q->head = front->next;
   del(front->data);
   free(front);
   q->length--;
   return data;
}

//returns the data at the front of a given queue
item getHead(Queue q) {

   assert(q != NULL);
   assert(isQueueEmpty(q) == FALSE);
   return q->head->data;
}

//returns the length of a given queue
unsigned long getQueueLength(Queue q) {

   assert(q != NULL);
   return q->length;
}

//indicates whether a given queue is empty
bool isQueueEmpty(Queue q) {

   assert(q != NULL);
   return q->length == 0;
}
