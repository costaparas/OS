#include <synch.h>

#include "barglass.h"

#ifndef BAR_H
#define BAR_H

/* struct barorder is the main type referred to in the code. It must
   be preserved as noted for our later testing to work */
struct barorder {
	unsigned int requested_bottles[DRINK_COMPLEXITY]; /* Do not change */
	int go_home_flag;                                 /* Do not change */
	struct glass glass;                               /* Do not change */

	/* This struct can be extended with your own entries below here */ 
	struct semaphore *order_ready; /* used to block until order ready */
};

#endif
