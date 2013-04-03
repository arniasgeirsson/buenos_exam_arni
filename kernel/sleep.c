#include "kernel/sleep.h"
#include "kernel/thread.h"
#include "kernel/config.h"
#include "kernel/assert.h"
#include "drivers/metadev.h"

/* not needed */
sleeping_thread_t *sleeping_thread_table[CONFIG_MAX_THREADS];

/* not needed */
void sleep_init_sleeping_table(void)
{
  int i;
  for (i=0; i < CONFIG_MAX_THREADS; i++) {
    sleeping_thread_table[i].msec = -1;
  }
}

/* not needed */
int sleep_get_empty_entry(void)
{
  int i;
  for (i=0; i < CONFIG_MAX_THREADS; i++) {
    if (sleeping_thread_table[i].msec == -1)
      return i;
  }
  return -1;
}

void thread_sleep(int msec)
{
  if (msec <= 0)
    return;

  thread_table_t *current_thread = thread_get_current_thread_entry();

  /* prepare thread */
  current_thread.state = THREAD_SLEEPING_TIME;
  /* remove next */
  current_thread.next = -1; /*?*/

  /* set numbers */
  current_thread.msec = msec;
  current_thread.msec_start = (int)rtc_get_msec();

  /* reschedule */
  thread_switch(); /* or use yield? probly not */
}
