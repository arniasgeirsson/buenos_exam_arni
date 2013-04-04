#include "tests/lib.h"

int main(void)
{
  int rc;
  int total_sems = 0;
  usr_sem_t tmp;
  while (1) {
    rc = syscall_sem_create(&tmp,1);
    if (rc == 0)
      total_sems++;
    else 
      break;
    tmp++;
  }
  printf("Child 1: Could not create more than %d sems, as a check the last rc was %d, expected %d\n",total_sems,rc,-2);
  syscall_exit(total_sems);
  return 0;
}
