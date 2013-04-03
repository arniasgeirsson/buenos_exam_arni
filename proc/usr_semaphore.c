#include "proc/usr_semaphore.h"
#include "proc/process.h"
#include "kernel/semaphore.h"
#include "lib/debug.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"

spinlock_t usr_semaphore_slock;

void usr_semaphore_init(void)
{
  int i;

  spinlock_reset(&usr_semaphore_slock);
  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    usr_semaphore_table[i].owner_pid = -1;
  }
}

void usr_semaphore_process_died(process_id_t pid)
{
  KERNEL_ASSERT(pid > -1 && pid < PROCESS_MAX_PROCESSES);
  int i;
  interrupt_status_t intr_status;

  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);

  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == pid) {
      semaphore_destroy(usr_semaphore_table[i].kernel_sem);
      usr_semaphore_table[i].owner_pid = -1;
    }
  }
  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);
}

int usr_semaphore_create(usr_sem_t *sem, int val)
{
  interrupt_status_t intr_status;
  semaphore_t *k_semaphore;
  process_id_t pid;
  int i;

  if (val < 0)
    return USR_SEMAPHORE_ERROR_VAL_NEGATIVE;

  sem = sem; /* check that sem is already in memory */
  
  k_semaphore = semaphore_create(val);

  if (k_semaphore == NULL)
    return USR_SEMAPHORE_ERROR_KERNEL_SEM_NULL;
  
  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);

  pid = process_get_current_process();  

  for (i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == -1) {
      break;
    }
  }
  
  if (i == CONFIG_MAX_SEMAPHORES) {
    spinlock_release(&usr_semaphore_slock);
    _interrupt_set_state(intr_status);
    return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
  }

  usr_semaphore_table[i].owner_pid = pid;
  usr_semaphore_table[i].user_sem = sem;
  usr_semaphore_table[i].kernel_sem = k_semaphore;

  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);
  return 0;
}

int usr_semaphore_sem_exist(usr_sem_t *sem)
{
  interrupt_status_t intr_status;
  process_id_t pid;
  int i;

  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);
  pid  = process_get_current_process();
  
  for (i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == pid
	&& usr_semaphore_table[i].user_sem == sem)
      {
	break;
      }
  }
  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);

  if (i == CONFIG_MAX_SEMAPHORES) {
    return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
  }
  return i;
}

int usr_semaphore_P(usr_sem_t *sem)
{
  int index = usr_semaphore_sem_exist(sem);
  if (index > -1) {
    semaphore_P(usr_semaphore_table[index].kernel_sem);
    return 0;
  }
  return index;
}

int usr_semaphore_V(usr_sem_t *sem)
{
  int index = usr_semaphore_sem_exist(sem);
  if (index > -1) {
    semaphore_V(usr_semaphore_table[index].kernel_sem);
    return 0;
  }
  return index;
}
