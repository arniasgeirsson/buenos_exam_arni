#include "tests/lib.h"

#define NUM_THREADS 5

static int threads[NUM_THREADS];
static int A[NUM_THREADS];
static volatile int done[NUM_THREADS];

uint32_t gg;

void test(int i)
{
  if (i==1 || i == 3) syscall_sleep(2000);
  syscall_sem_p((usr_sem_t*)gg);
  printf("Thread %d doing its thing!\n", i);
  A[i] = i*2;
  done[i] = 1;
  syscall_sem_v((usr_sem_t*)gg);
}

int main()
{
  int i;
  gg = 100;
  syscall_sem_create((usr_sem_t*)gg,1);
  for (i = 0; i < NUM_THREADS; ++i)
  {
    A[i] = 0;
    done[i] = 0;
    threads[i] = syscall_fork(&test, i);
  }
  for (i = 0; i < NUM_THREADS; ++i)
  {
    while (!done[i])
      ;
  }
  for (i = 0; i < NUM_THREADS; ++i)
  {
    printf("Thread %d: %d\n", i, A[i]);
  }
  return 0;
}
