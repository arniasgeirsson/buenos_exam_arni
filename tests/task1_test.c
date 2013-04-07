#include "tests/lib.h"

static const char prog[] = "[disk]totalsems";

usr_sem_t sem, sem4;

int runningthreads;
int waiting;

void take_sem(int i)
{
  int rc;
  rc = syscall_sem_p(&sem);
  printf("- 3.1: Thread #%d toke the semaphore!     rc = %d [%d]\n",i,rc,0);
  printf("- 3.1: Thread #%d released the semaphore! rc = %d [%d]\n",i,rc,0);
  runningthreads--;
  rc = syscall_sem_v(&sem);
}

void take_sem2(int i)
{
  int rc;
  printf("- 2.6.1: New thread %d is going to take the semaphore.%s\n",i,"");
  waiting = 1;
  rc = syscall_sem_p(&sem4);
  printf("- 2.6.2: New thread %d toke the semaphore!%s\n",i,"");
  if (rc != 0) syscall_halt(); /* Something is wrong. */
  rc = syscall_sem_v(&sem4);
  if (rc != 0) syscall_halt(); /* Something is wrong. */
  /* Let main know we are done with this test. */
  waiting = 0;
}

int main(void)
{
  int rc;
  int i;
  int v;
  int numthreads;
  usr_sem_t sem1, sem2, sem3, sem5;

  /* Section 1 - Checking error messages. */
  printf("\nTesting assignment 1 - section 1%s\n","");

  /* Test 1: Trying to take an unitialized semaphore. */
  printf("Test 1: Trying to take an unitialized semaphore.%s\n","");
  rc = syscall_sem_p(&sem1);
  printf("- 1.1: Return value from syscall_sem_p is %d [%d]\n",rc,-3);

  /* Test 2: Trying to release an unitialized semaphore. */
  printf("Test 2: Trying to release an unitialized semaphore.%s\n","");
  rc = syscall_sem_v(&sem1);
  printf("- 1.2: Return value from syscall_sem_v is %d [%d]\n",rc,-3);

  /* Test 3: Trying to create a semaphore with a negative value. */
  printf("Test 3: Trying to create a semaphore with a negative value.%s\n","");
  rc = syscall_sem_create(&sem1,-1);
  printf("- 1.3: Return value from syscall_sem_create is %d [%d]\n",rc,-1);

  /* Test 4: Trying to create a semaphore that is already in use. */
  printf("Test 4: Trying to create a semaphore that is already in use.%s\n","");
  rc = syscall_sem_create(&sem1,1);
  if (rc != 0) syscall_halt(); /* Something is wrong. */
  rc = syscall_sem_create(&sem1,1);
  printf("- 1.4: Return value from syscall_sem_create is %d [%d]\n",rc,-5);

  /* Section 2 - Using a working semaphore. */
  printf("\nTesting assignment 1 - section 2%s\n","");

  /* Test 1: Trying to create a semaphore with a positive value. */
  printf("Test 1: Trying to create a semaphore with a positive value.%s\n","");
  rc = syscall_sem_create(&sem2,1);
  printf("- 2.1: Return value from syscall_sem_create is %d [%d]\n",rc,0);

  /* Test 2: Trying to take an initialized semaphore. */
  printf("Test 2: Trying to take an initialized semaphore.%s\n","");
  rc = syscall_sem_p(&sem2);
  printf("- 2.2: Return value from syscall_sem_p is now %d [%d]\n",rc,0);

  /* Test 3: Trying to release an initialized semaphore. */
  printf("Test 3: Trying to release an initialized semaphore.%s\n","");
  rc = syscall_sem_v(&sem2);
  printf("- 2.3: Return value from syscall_sem_v is now %d [%d]\n",rc,0);

  /* Test 4: Trying to create a semaphore with a positive value v, v > 1
     And trying to take it v times before releasing it v times. */
  printf("Test 4: Trying to use a valid semaphore with value v.%s\n","");
  v = 4;
  rc = syscall_sem_create(&sem3,v);
  printf("- 2.4.1: I have now created sem3 with the value of %d.\n",v);

  if (rc != 0) syscall_halt(); /* Something is wrong. */
  for (i=0; i < v; i++) {
    rc = syscall_sem_p(&sem3);
    if (rc != 0) syscall_halt(); /* Something is wrong. */
  }
  printf("- 2.4.2: I have now taken sem3 %d times, and nothing has gone wrong.\n",v);

  for (i=0; i < v; i++) {
    rc = syscall_sem_v(&sem3);
    if (rc != 0) syscall_halt(); /* Something is wrong. */
  }
  printf("- 2.4.3: I have now released sem3 %d times, and nothing has gone wrong.\n",v);

  /* Test 5: Trying to create a semaphore with a value of 0. */
  printf("Test 5: Trying to create a semaphore with a value of 0.%s\n","");
  rc = syscall_sem_create(&sem4,0);
  printf("- 2.5: Return value from syscall_sem_create is %d [%d]\n",rc,0);

  /* Test 6: Trying to let a thread take the semaphore with a value of 0. */
  printf("Test 6: Trying to let a thread take the semaphore created with a value of 0.%s\n","");
  waiting = 0;
  rc = syscall_fork(&take_sem2,0);
  if (rc != 0) syscall_halt(); /* Something is wrong. */
  
  while (waiting == 0) ;
  printf("- 2.6.3: Main released the semaphore.%s\n","");
  syscall_sem_v(&sem4);
  while (waiting == 1) ;

  /* Section 3 - Letting other sibling threads use share a semaphore. */
  printf("\nTesting assignment 1 - section 3%s\n","");

  /* Test 1: Trying to create a semaphore sem and create and let a number of threads
     within this process and let them use sem. */
  printf("Test 1: Trying to let other threads use a created semaphore.%s\n","");
  
  rc = syscall_sem_create(&sem,1);
  if (rc != 0) syscall_halt(); /* Something is wrong. */
  
  numthreads = 6;
  runningthreads = numthreads;  

  for (i=0; i < numthreads; i++) {
    if (syscall_fork(&take_sem,i) < 0) {
      printf("- 3.1: Not enough threads could be created, halting!\n");
      syscall_halt();
    }
  }
  while (runningthreads != 0);


  /* Section 4 - Making sure semaphores gets destroyed properly. */
  printf("\nTesting assignment 1 - section 4%s\n","");

  /* Test 1: Trying to let a new process create as many semaphores as possible,
     and return the amount of semaphores created.
     I assume that the process will create enough semaphores to take up all
     the kernel semaphores, so that no one can create a new user semaphore. */
  printf("Test 1: Trying to create as many semaphores as possible in child process.%s\n","");

  int ts = syscall_join(syscall_exec(prog));
  if (ts < 0) syscall_halt(); /* Something is wrong. */

  /* Now that the process above is dead, I expect that my clean-up handling
     in my implementation of user semaphores has freed up all the semaphores
     that the child process used. */
  /* Test 2: Trying to create as many semaphores as the child process could create,
     showing that all the semaphores created by that process is now free. */
  printf("Test 2: Trying to create as many semaphores as the child process from Test 1.%s\n","");

  int total_sems = 0;
  while (1) {
    rc = syscall_sem_create(&sem5,1);
    if (rc == 0)
      total_sems++;
    else 
      break;
    sem5++;
  }
  printf("- 4.2: Could create %d semaphores [%d], rc was %d [%d]\n\n",total_sems,ts,rc,-2);

  return 0;
}
