#include <types.h>

#include "item.h"

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue *Queue;

//queue initialisation
Queue create_queue(void);
void dispose_queue(Queue q);

//queue progression
void enqueue(Queue q, item data);
item dequeue(Queue q);

//queue query
item get_head(Queue q);
unsigned long get_queue_length(Queue q);
bool is_queue_empty(Queue q);

#endif
