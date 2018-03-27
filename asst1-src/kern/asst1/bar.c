#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"
#include "Queue.h"
#include "quicksort.h"

/* Declare any globals you need here (e.g. locks, etc...) */
Queue pending_orders;
struct lock *que_lock;
struct cv *order_made;
struct lock *bottle_locks[NBOTTLES];

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */
void order_drink(struct barorder *order) {
	order->order_ready = cv_create("order ready");
	lock_acquire(que_lock);
	enqueue(pending_orders, order);
	lock_release(que_lock);
	cv_signal(order_made, que_lock);
	cv_destroy(order->order_ready);
}

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */
struct barorder *take_order(void) {
	lock_acquire(que_lock);
	cv_wait(order_made, que_lock);
	struct barorder *ret = dequeue(pending_orders);
	fill_order(ret);
	lock_release(que_lock);
	return ret;
}

/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */
void fill_order(struct barorder *order) {

	/* add any sync primitives you need to ensure mutual exclusion
	holds as described */
	quicksort(order->requested_bottles, 0, DRINK_COMPLEXITY - 1, DRINK_COMPLEXITY);
	for (int i = 0; i < DRINK_COMPLEXITY; ++i) {
		lock_acquire(order->requested_bottles[i]);
	}

	/* the call to mix must remain */
	mix(order);

	for (int i = 0; i < DRINK_COMPLEXITY; ++i) {
		lock_release(order->requested_bottles[i]);
	}

}

/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */
void serve_order(struct barorder *order) {
	(void) order; /* avoid a compiler warning, remove when you start */
}

/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */

/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */
void bar_open(void) {
	pending_orders = create_queue();
	que_lock = lock_create("queue lock");
	if (que_lock == NULL) panic("%s: que_lock create failed", __FILE__);
	order_made = cv_create("order made");
	if (order_made == NULL) panic("%s: order_made create failed", __FILE__);
	for (int i = 0; i < NBOTTLES; ++i) {
		bottle_locks[i] = lock_create("bottle lock");
		if (bottle_locks[i] == NULL) panic("%s: bottle_lock %d create failed", __FILE__, i);
	}
}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */
void bar_close(void) {
	dispose_queue(pending_orders);
	lock_destroy(que_lock);
	cv_destroy(order_made);
	for (int i = 0; i < NBOTTLES; ++i) {
		lock_destroy(bottle_locks[i]);
	}
}
