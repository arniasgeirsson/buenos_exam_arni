#include "tests/lib.h"

#define NUM_THREADS 10

static volatile int done[NUM_THREADS];
static volatile int order[NUM_THREADS];

usr_sem_t sem;
int sleeptime;

void insert_id(int id)
{
  int i;
  for (i=0; i < NUM_THREADS; i++) {
    if (order[i] == -1) {
      order[i] = id;
      return;
    }
  }
}

void p_v_sem(char *testnr, int i)
{
  int rc;
  rc = syscall_sem_p(&sem);
  if (rc != 0) syscall_halt(); /* Something is wrong */

  printf("%s: Thread %d is done!\n",testnr,i);
  insert_id(i);
  done[i] = 1;

  rc = syscall_sem_v(&sem);
  if (rc != 0) syscall_halt(); /* Something is wrong */
}

void wo_yield(int i)
{
  p_v_sem("- 1.2.2",i);
}

void with_yield(int i)
{
  int rc;
  if (i% 2 == 0) {
    rc = syscall_yield();
    if (rc != 0) syscall_halt(); /* Something is wrong */
    rc = syscall_yield();
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  p_v_sem("- 1.2.5",i);
}

void wo_sleep(int i)
{
  p_v_sem("- 2.4.2",i);
}

void with_sleep(int i)
{
  int rc;
  if (i% 2 == 0) {
    rc = syscall_sleep(sleeptime);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  p_v_sem("- 2.4.3",i);
}

int check_order_asc()
{
  int i;
  for (i=1; i < NUM_THREADS; i++) {
    if (order[i-1] > order[i]) return 0;
  }
  return 1;
}

void wait_loop()
{
  int i;
  for (i = 0; i < NUM_THREADS; ++i)
    {
      while (!done[i]) ;
    }
}

int main(void)
{
  int rc;
  int i;
  int start,end;

  rc = syscall_sem_create(&sem,1);
  if (rc != 0) syscall_halt(); /* Something is wrong */

  /* Section 1 - Testing yield functionality. */
  printf("\nTesting assignment 2 - section 1%s\n","");
  
  /* Test 1: Trying to yield while I am the only thread running. */
  printf("Test 1: Trying to yield while I am the only thread running.%s\n","");

  rc = syscall_yield();
  printf("- 1.1: Return value from syscall_yield is %d [%d]\n",rc,-1);
  
  /* Test 2: Trying to run a sequence of new threads, with and without
     yielding to see if expected behavior occurs. */
  printf("Test 2: Trying to see if syscall_yield changes order in a sequence of threads.%s\n","");

  printf("- 1.2.1: Creating %d threads and no call syscall_yield.\n",NUM_THREADS);
  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&wo_yield,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("- 1.2.3: Order of threads is ascending %d [%d]\n",rc,1);

  printf("- 1.2.4: Creating %d threads and some call syscall_yield.\n",NUM_THREADS);
  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&with_yield,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("- 1.2.6: Order of threads is ascending %d [%d]\n",rc,0);

  /* Section 2 - Testing sleep functionality. */
  printf("\nTesting assignment 2 - section 2%s\n","");
  
  /* Test 1: Trying to sleep for a negative amount of time. */
  printf("Test 1: Trying to sleep for a negative amount of time.%s\n","");

  sleeptime = -40;
  rc = syscall_sleep(sleeptime);
  printf("- 2.1: Return value from sleep is %d [%d]\n",rc,-1);

  /* Test 2: Trying to sleep for 0 milliseconds. */
  printf("Test 2: Trying to sleep for 0 milliseconds.%s\n","");

  sleeptime = 0;
  rc = syscall_sleep(sleeptime);
  printf("- 2.2: Return value from sleep is %d [%d]\n",rc,-1);

  /* Test 3: Trying to sleep for a positive amount of time. */
  printf("Test 3: Trying to sleep for a positive amount of time.%s\n","");

  sleeptime = 20;
  start = syscall_get_time_from_start();

  rc = syscall_sleep(sleeptime);
  end = syscall_get_time_from_start();
  printf("- 2.3: Return value from sleep is %d [%d] and actual sleep time %d [>=%d]\n",rc,0,end-start,sleeptime);

  /* Test 4: Trying to run a sequence of new threads, with and without
     sleeping to see if expected behavior occurs. */
  printf("Test 4: Trying to see if syscall_sleep changes order in a sequence of threads.%s\n","");
  
  sleeptime = 100;
  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&wo_sleep,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("- 2.4.1: Order of threads is ascending %d [%d]\n",rc,1);

  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&with_sleep,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("- 2.4.4: Order of threads is ascending %d [%d]\n",rc,0);

  return 0;
}
