#ifndef BUENOS_PROC_SEMAPHORE
#define BUENOS_PROC_SEMAPHORE

#include "lib/types.h"
#include "kernel/config.h"
#include "proc/process.h"
#include "kernel/semaphore.h"

#define USR_SEMAPHORE_ERROR_VAL_NEGATIVE -1
#define USR_SEMAPHORE_ERROR_KERNEL_SEM_NULL -2
#define USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST -3

typedef struct {
  process_id_t owner_pid;
  usr_sem_t *user_sem;
  semaphore_t *kernel_sem;
} sem_pair_t;

sem_pair_t usr_semaphore_table[CONFIG_MAX_SEMAPHORES];

void usr_semaphore_init(void);
int usr_semaphore_create(usr_sem_t *sem, int val);
int usr_semaphore_P(usr_sem_t *sem);
int usr_semaphore_V(usr_sem_t *sem);
void usr_semaphore_process_died(process_id_t pid);

#endif
