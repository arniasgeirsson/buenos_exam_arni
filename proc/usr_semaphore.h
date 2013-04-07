#ifndef BUENOS_PROC_SEMAPHORE
#define BUENOS_PROC_SEMAPHORE

#include "lib/types.h"
#include "proc/process.h"
#include "kernel/semaphore.h"

#define USR_SEMAPHORE_SUCCES 0
#define USR_SEMAPHORE_ERROR_VAL_NEGATIVE -1
#define USR_SEMAPHORE_ERROR_KERNEL_SEM_NULL -2
#define USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST -3
#define USR_SEMAPHORE_ERROR_NO_FREE_SEM -4
#define USR_SEMAPHORE_ERROR_SEM_ALREADY_EXISTS -5

/* A struct used to hold the information of which
   kernel semaphores are mapped to which user
   semaphores. */
typedef struct {

  /* The addresse of the user semaphore. */
  uint32_t sem_addr;

  /* The pid of process owing the user semaphore. */
  process_id_t owner_pid;
 
  /* The mapped kernel semaphore. */
  semaphore_t *kernel_sem;

} sem_pair_t;

/* Initializes the user semaphore table. */
void usr_semaphore_init(void);

/* Removes all user semaphores owned by a process pid
   from the user semaphore table. */
void usr_semaphore_process_died(process_id_t pid);

/* Initializes a user semaphore. */
int usr_semaphore_create(usr_sem_t *sem, int val);

/* Procures a user semaphore. */
int usr_semaphore_P(usr_sem_t *sem);

/* Vacates a user semaphore. */
int usr_semaphore_V(usr_sem_t *sem);

#endif
