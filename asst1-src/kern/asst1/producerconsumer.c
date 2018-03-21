#include <types.h>
#include <synch.h>
#include <lib.h>

#include "producerconsumer_driver.h"

//bounded buffer for the producer/consumer
static struct _buffer {
	struct pc_data data[BUFFER_SIZE];
	uint32_t head, tail; // tail is next available slot
} buffer;

//synchronization primitives for the producer/consumer
static struct cv *has_capacity;
static struct cv *has_data;
static struct lock *buffer_lock;

/* consumer_receive() is called by a consumer to request more data. It
 * should block on a sync primitive if no data is available in your
 * buffer.
 */
struct pc_data consumer_receive(void) {
	lock_acquire(buffer_lock);
	while ((buffer.tail - buffer.head) % BUFFER_SIZE == 0)
		cv_wait(has_data, buffer_lock);

	struct pc_data thedata = buffer.data[buffer.head++];
	buffer.head %= BUFFER_SIZE;

	cv_signal(has_capacity, buffer_lock);
	lock_release(buffer_lock);

	return thedata;
}

/* procucer_send() is called by a producer to store data in your
 * bounded buffer.
 */
void producer_send(struct pc_data item) {
	lock_acquire(buffer_lock);
	while ((buffer.tail - buffer.head) % BUFFER_SIZE == BUFFER_SIZE)
		cv_wait(has_capacity, buffer_lock);

	buffer.data[buffer.tail++] = item;
	buffer.tail %= BUFFER_SIZE;

	cv_signal(has_data, buffer_lock);
	lock_release(buffer_lock);
}

/* Perform any initialisation (e.g. of global data) you need
 * here. Note: You can panic if any allocation fails during setup
 */
void producerconsumer_startup(void) {
	has_capacity = cv_create("has capacity");
	has_data = cv_create("has data");
	buffer_lock = lock_create("buffer lock");

	if (has_capacity == NULL)
		panic("%s: has_capacity create failed", __FILE__);
	if (has_data == NULL)
		panic("%s: has_data create failed", __FILE__);
	if (buffer_lock == NULL)
		panic("%s: buffer_lock create failed", __FILE__);

	// Initialise buffer props
	buffer.head = 0;
	buffer.tail = 0;
}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void) {
	cv_destroy(has_capacity);
	cv_destroy(has_data);
	lock_destroy(buffer_lock);
}
