#include "item.h"

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue *Queue;

//queue initialisation
Queue createQueue(void);
void  disposeQueue(Queue q);

//queue progression
void enqueue(Queue q, item data);
item dequeue(Queue q);

//queue query
item getHead(Queue q);
unsigned long getQueueLength(Queue q);
bool isQueueEmpty(Queue q);

#endif
