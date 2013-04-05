#include "tests/lib.h"

int main(void)
{
  usr_sem_t addr = 100;
  int rc;
  /* Trying to use a semaphore created by another process.*/
  rc = syscall_sem_v(&addr);
  syscall_exit(rc);
  return 0;
}
