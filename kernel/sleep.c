#include "kernel/sleep.h"
#include "kernel/thread.h"
#include "lib/debug.h"
#include "lib/libc.h"
#include "kernel/scheduler.h"

void thread_sleep(int msec)
{
  if (msec <= 0)
    return;
  //kprintf("sadasddsa###################   %d\n",msec);
  scheduler_add_time_sleeping_thread(msec);

  /* reschedule */
  thread_switch();
}
