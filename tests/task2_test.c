#include "tests/lib.h"

#define NUM_THREADS 10

static volatile int done[NUM_THREADS];
static volatile int order[NUM_THREADS];

usr_sem_t sem;

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
  p_v_sem("1.2.2",i);
}

void with_yield(int i)
{
  int rc;
  if (i% 2 == 1) {
   rc = syscall_yield();
   if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  p_v_sem("1.2.3",i);
}

void wo_sleep(int i)
{
  p_v_sem("2.4.2",i);
}

void with_sleep(int i)
{
  int rc;
  if (i% 2 == 1) {
   rc = syscall_sleep(100);
   if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  p_v_sem("2.4.3",i);
}

int check_order_asc()
{
  int i;
  for (i=1; i < NUM_THREADS; i++) {
    if (order[i-1] > order[i]) return -1;
  }
  return 0;
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
  rc = syscall_sem_create(&sem,1);
  if (rc != 0) syscall_halt(); /* Something is wrong */

  /* Section 1 - Testing yield functionality. */
  printf("Testing assignment 2 - section 1%s\n","");
  
  /* Test 1: Trying to yield while I am the only thread running. */
  rc = syscall_yield();
  printf("1.1: Return code from syscall_yield when I am alone is: %d [%d]\n",rc,-1);
  
  /* Test 2: Trying to run a sequence of new threads, with and without
     yielding to see if expected behavior occurs. */
  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&wo_yield,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("1.2.1: Order of threads is ascending %d [%d]\n",rc,0);

  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&with_yield,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("1.2.4: Order of threads is ascending %d [%d]\n",rc,-1);

  /* Section 2 - Testing sleep functionality. */
  printf("Testing assignment 2 - section 2%s\n","");
  
  int time;
  /* Test 1: Trying to sleep for a negative amount of time. */
  time = -40;
  rc = syscall_sleep(time);
  printf("2.1: Return code from sleep is %d [%d]\n",rc,-1);

  /* Test 2: Trying to sleep for 0 milliseconds. */
  time = 0;
  rc = syscall_sleep(time);
  printf("2.2: Return code from sleep is %d [%d]\n",rc,-1);

  /* Test 3: Trying to sleep for a positive amount of time. */
  time = 20;
  int start = syscall_get_time_from_start();

  rc = syscall_sleep(time);
  int end = syscall_get_time_from_start();
  printf("2.3: Return code from sleep is %d [%d] and actual sleep time %d [%d]\n",rc,0,end-start,time);

  /* Test 4: like test 2 in section 1. */
  /* Test 2: Trying to run a sequence of new threads, with and without
     yielding to see if expected behavior occurs. */
  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&wo_sleep,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("2.4.1: Order of threads is ascending %d [%d]\n",rc,0);

  for (i=0; i < NUM_THREADS; i++) {
    done[i]= 0;
    order[i] = -1;
    rc = syscall_fork(&with_sleep,i);
    if (rc != 0) syscall_halt(); /* Something is wrong */
  }
  
  wait_loop();
  rc = check_order_asc();
  printf("2.4.4: Order of threads is ascending %d [%d]\n",rc,-1);

  return 0;
}
