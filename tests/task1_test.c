#include "tests/lib.h"

usr_sem_t sem;
usr_sem_t sem1;
usr_sem_t sem2;
int runningthreads;

void take_sem(int i)
{
  int rc;
  rc = syscall_sem_p(&sem);
  printf("Thread #%d toke the semaphore! rc = %d, was expecting %d\n",i,rc,0);
  printf("Thread #%d released the semaphore! rc = %d, was expecting %d\n",i,rc,0);
  runningthreads--;
  rc = syscall_sem_v(&sem);
}

void control_sem(int i)
{
  syscall_sem_p(&sem);
  printf("Controlthread #%d toke the semaphore!\n",i);
}

int main(void)
{
  int rc;
  /* Initial test, testing trying to use a wrong semaphore */
  
  /* Trying to create a semaphore on uninitialized semaphore
     Expecting an error value. */
  /* When is it unitialized/not in user memory space? */
  //rc = syscall_sem_create(44,1); -> this results in type error.
  //printf("1: Return value from sem create is %d, was expecting %d\n",rc,-4);

  /* Trying to create a sem with something thats not a sem
     Expecting an error value.
     How is that possible? */
  /* No, type error. */

  /* Trying to take an unitialized semaphore
     is similar to trying to use a semaphore that does not belong to you. */
  rc = syscall_sem_p(&sem1);
  printf("1: Return value from sem p is %d, was expecting %d\n",rc,-3);
  /* Trying to take an unitialized semaphore
     is similar to trying to use a semaphore that does not belong to you. */
  rc = syscall_sem_v(&sem1);
  printf("2: Return value from sem v is %d, was expecting %d\n",rc,-3);

  /* initialize the semaphore value */
  rc = syscall_sem_create(&sem1,1);
  printf("3: Return value from sem create is %d, was expecting %d\n",rc,0);

  /* again p */
  rc = syscall_sem_p(&sem1);
  printf("4: Return value from sem p is now %d, was expecting %d\n",rc,0);

  /* again v */
  rc = syscall_sem_v(&sem1);
  printf("5: Return value from sem v is now %d, was expecting %d\n",rc,0);

  /* Try and create again */
  rc = syscall_sem_create(&sem1,1);
  printf("6: Return value from sem create is %d, was expecting %d\n",rc,-5);


  /* Try and create with negative value */
  rc = syscall_sem_create(&sem2,-1);
  printf("7: Return value from sem create is %d, was expecting %d\n",rc,-1);

  /* Try and create with value 0 */
  /* Is allowed and will create a semaphore that is already
     locked by the caller */
  rc = syscall_sem_create(&sem2,0);
  printf("8: Return value from sem create is %d, was expecting %d\n",rc,0);

  /* Try to execute a new process, that creates a semaphore,
     and then let this process try and use it. */

  /* Try and use a correct semaphore with different values
     I expect that all the simulation look alike */
  /* This test also shows that a thread can use a semaphore,
     which was created by another thread in that process. */
  int value = 20;
  int controlthreads = value - 1;
  
  rc = syscall_sem_create(&sem,value);
  printf("9: Return value from sem create is %d, was expecting %d\n",rc,0);
  
  int numthreads = 6;
  runningthreads = numthreads;  
  int i;
  for (i=0; i < controlthreads; i++) {
    printf("Master took a control ticket %d\n",i);
    syscall_sem_p(&sem);
  }

  for (i=0; i < numthreads; i++) {
    syscall_fork(&take_sem,i);
  }
  while (runningthreads != 0);

  /* Testing if semaphores gets destroyed properly */

  /* Since I know there is a bound to how many kernel semaphores can be
     in use at a time, I can first try and create semaphores until I
     can't no more. So I know the number of semaphores*/
  syscall_join(syscall_exec("[disk]test5"));
  
  /* Then I can execute a new process that creates a specific amount of
     semaphores, then dies, and if I can now create as many semaphores as
     before then they were destroyed properly.*/
  usr_sem_t sem4;
  printf("Can I now create a new semaphore? -> %d\n",syscall_sem_create(&sem4,1)==0);
  return 0;
}
