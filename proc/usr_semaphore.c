#include "proc/usr_semaphore.h"
#include "proc/process.h"
#include "kernel/semaphore.h"
#include "lib/debug.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "lib/libc.h"

spinlock_t usr_semaphore_slock;

sem_pair_t usr_semaphore_table[CONFIG_MAX_SEMAPHORES];

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

/* Expects that you have locked the table */
int usr_semaphore_find_empty_spot(void)
{
  int i;
  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == -1)
      return i;
  }
  return -1;
}

int usr_semaphore_sem_exist(usr_sem_t *sem)
{
  return usr_semaphore_table[*sem].owner_pid == process_get_current_process()
    && usr_semaphore_table[*sem].sem_addr == (uint32_t)sem;
}

int usr_semaphore_create(usr_sem_t *sem, int val)
{
  interrupt_status_t intr_status;
  semaphore_t *k_semaphore;
  process_id_t pid;
  int index;

  if (val < 0)
    return USR_SEMAPHORE_ERROR_VAL_NEGATIVE;

  //kprintf("sem_create sem-addr is %d, sem-value is %d\n",sem,(uint32_t)*sem);
  /* Does this semaphore already exist? */
  if (usr_semaphore_sem_exist(sem)) {
    return USR_SEMAPHORE_ERROR_SEM_ALREADY_EXISTS;
  }
  
  k_semaphore = semaphore_create(val);

  if (k_semaphore == NULL)
    return USR_SEMAPHORE_ERROR_KERNEL_SEM_NULL;
  
  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);

  pid = process_get_current_process();  

  index = usr_semaphore_find_empty_spot();
  if (index < 0) {
    spinlock_release(&usr_semaphore_slock);
    _interrupt_set_state(intr_status);
    return USR_SEMAPHORE_ERROR_NO_FREE_SEM;
  }

  *sem = index;
  //kprintf("sem_create an empty spot was found at index %d (%d)\n",index,(uint32_t)*sem);
  usr_semaphore_table[index].owner_pid = pid;
  usr_semaphore_table[index].sem_addr = (uint32_t)sem;
  usr_semaphore_table[index].kernel_sem = k_semaphore;

  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);
  return USR_SEMAPHORE_SUCCES;
}

int usr_semaphore_P(usr_sem_t *sem)
{
  //kprintf("p: trying to take sem with addr %d, and value %d\n",sem,(uint32_t)*sem);
  if (usr_semaphore_sem_exist(sem)) {
    semaphore_P(usr_semaphore_table[*sem].kernel_sem);
    return USR_SEMAPHORE_SUCCES;
  }
  return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
}

int usr_semaphore_V(usr_sem_t *sem)
{
  //kprintf("v: trying to take sem with addr %d, and value %d\n",sem,(uint32_t)*sem);
  if (usr_semaphore_sem_exist(sem)) {
    semaphore_V(usr_semaphore_table[*sem].kernel_sem);
    return USR_SEMAPHORE_SUCCES;
  }
  return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
}
