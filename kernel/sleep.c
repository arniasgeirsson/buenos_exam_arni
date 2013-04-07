#include "kernel/sleep.h"
#include "kernel/thread.h"
#include "kernel/scheduler.h"

/**
 * Lets the calling thread go into sleep for a
 * specified amount of milliseconds.
 *
 * @param msec The specified sleep time in milliseconds.
 *
 * @see scheduler_add_time_sleeping_thread
 */
void sleep_thread_sleep(int msec)
{
  /* Make sure that msec is positive. */
  if (msec <= 0)
    return;
  
  /* Add the calling thread to the linked list
     of sleeping threads in the scheduler. */
  scheduler_add_time_sleeping_thread(msec);

  /* reschedule to start the sleeping period. */
  thread_switch();
}
