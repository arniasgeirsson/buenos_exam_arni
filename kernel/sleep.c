#include "kernel/sleep.h"
#include "kernel/thread.h"
#include "kernel/config.h"
#include "drivers/metadev.h"
#include "lib/debug.h"
#include "kernel/assert.h"
#include "lib/libc.h"

void thread_sleep(int msec)
{
  if (msec <= 0)
    return;
  /* call it as early as possible? */
  current_thread->msec_start = (int)rtc_get_msec();

  thread_table_t *current_thread = thread_get_current_thread_entry();

  /* prepare thread */
  current_thread->state = THREAD_SLEEPING_TIME;
  /* remove next */
  current_thread->next = -1; /*?*/

  /* set numbers */
  /* debugging or printing in here fucks everything up. wtf? */
  //kprintf("current_thread->msec = %d\n",current_thread->msec);
  KERNEL_ASSERT(current_thread->msec == -1);
  current_thread->msec = msec;
  //DEBUG("task1_debug","current_thread->msec = %d\n",6);//current_thread->msec);
  /* reschedule */
  thread_switch(); /* or use yield? probly not */
}
