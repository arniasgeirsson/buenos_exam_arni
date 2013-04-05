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
  while (1) {
    rc = syscall_sem_create(&tmp,1);
    if (rc == 0)
      total_sems++;
    else 
      break;
    tmp++;
  }
  printf("4.1: New process could not create more than %d sems, as a check the last rc was %d [%d]\n",total_sems,rc,-2);

  /* Return the number of created semaphores to parent process. */
  syscall_exit(total_sems);
  return 0;
}
