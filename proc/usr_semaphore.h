#ifndef BUENOS_PROC_SEMAPHORE
#define BUENOS_PROC_SEMAPHORE

#include "lib/types.h"
#include "kernel/config.h"

typedef struct {
  int owner_pid;
  uint32_t user_sem;
  uint32_t kernel_sem;
} sem_pair_t;

sem_pair_t usr_semaphore_table[CONFIG_MAX_SEMAPHORES];

void usr_semaphore_init(void);
int usr_semaphore_create(uint32_t *sem, int val);
int usr_semaphore_P(uint32_t *sem);
int usr_semaphore_V(uint32_t *sem);

#endif
