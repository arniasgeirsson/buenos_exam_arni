#ifndef BUENOS_PROC_SEMAPHORE
#define BUENOS_PROC_SEMAPHORE

#include "lib/types.h"

typedef struct {
  uint32_t user_sem;
  uint32_t kernel_sem;
  uint32_t free;
} sem_pair_t;

int usr_semaphore_create(uint32_t *sem, int val);
int usr_semaphore_P(uint32_t *sem);
int usr_semaphore_V(uint32_t *sem);

#endif
