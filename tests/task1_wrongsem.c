#include "tests/lib.h"

int main(void)
{
  usr_sem_t addr;
  int rc;
  rc = syscall_sem_create(&addr,2);
  if (rc != 0) syscall_halt(); /* Something is wrong */
  syscall_exit(addr);
  return 0;
}
