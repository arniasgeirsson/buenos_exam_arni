#include "tests/lib.h"

int main(void)
{
  int rc;
  int total_sems = 0;
  usr_sem_t tmp;

  /* Create semaphores until something goes wrong.
     As the printf message below also suggests, then I
     expect syscall_sem_create will first be unsuccesfull when
     all kernel semaphores have been taking and the return code
     is -2. */
  /* The semaphores are created with a value of range 1-19,
     and some are procured for the sake of testing. */
  while (1) {
    rc = syscall_sem_create(&tmp,((int)tmp % 20)+1);
    if (rc == 0)
      total_sems++;
    else
      break;
    if (total_sems % 2 == 1) {
      rc = syscall_sem_p(&tmp);
      if (rc != 0) syscall_halt(); /* Something is wrong. */
    }
    tmp++;
  }
  printf("- 4.1: Child process could not create more than %d sems, as a check the last rc was %d [%d]\n",total_sems,rc,-2);

  /* Return the number of created semaphores to parent process. */
  syscall_exit(total_sems);
  return 0;
}
