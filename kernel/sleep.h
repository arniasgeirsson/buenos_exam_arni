#ifndef BUENOS_KERNEL_SLEEP
#define BUENOS_KERNEL_SLEEP

/* not needed */
typedef struct {
  int msec;
  int msec_start;
} sleeping_thread_t;

/* follow naming convention? */
void thread_sleep(int msec);

#endif
