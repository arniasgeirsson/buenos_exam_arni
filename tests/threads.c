#include "tests/lib.h"

#define NUM_THREADS 5

static int threads[NUM_THREADS];
static int A[NUM_THREADS];
static volatile int done[NUM_THREADS];

void test(int i)
{
  printf("Thread %d doing its thing!\n", i);
  A[i] = i*2;
  done[i] = 1;
}

int main()
{
  int i;

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
