#include "proc/usr_semaphore.h"
#include "proc/process.h"
#include "kernel/semaphore.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "kernel/config.h"

/**
 * The user semaphore table, it contains all the mapping between
 * a user semaphore and a kernel semaphore.
 */
sem_pair_t usr_semaphore_table[CONFIG_MAX_SEMAPHORES];

/**
 * A spinlock protecting the user semaphore table.
 */
spinlock_t usr_semaphore_slock;

/**
 * Initializes the user semaphore table by setting all
 * entries to free.
 * A free entry is marked by owner_pid is set to -1.
 */
void usr_semaphore_init(void)
{
  int i;

  spinlock_reset(&usr_semaphore_slock);
  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    usr_semaphore_table[i].owner_pid = -1;
  }
}

/**
 * Destroys all user semaphores owned by a given process.
 *
 * @param pid The id of the given process.
 *
 * @see semaphore_destoy
 */
void usr_semaphore_process_died(process_id_t pid)
{
  int i;
  interrupt_status_t intr_status;

  /* The pid must be valid. */
  KERNEL_ASSERT(pid > -1 && pid < PROCESS_MAX_PROCESSES);

  /* Disable interrupts and acquire the user semaphore table spinlock. */
  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);

  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == pid) {
      /* This is a semaphore owned by pid. */

      /* Destroy the mapped kernel semaphore. */
      semaphore_destroy(usr_semaphore_table[i].kernel_sem);
      /* Mark the table entry to be free. */
      usr_semaphore_table[i].owner_pid = -1;
    }
  }
  
  /* Release the spinlock and restore interrupt status state. */
  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);
}

/**
 * Looks through the user semaphore table for an free spot.
 *
 * @return The index of the first free spot.
 * @return -1 if the table is full.
 */
int usr_semaphore_find_empty_spot(void)
{
  int i;
  for (i=0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == -1)
      return i;
  }
  return -1;
}

/**
 * Checks if a user semaphore already exists in the user semaphore table.
 * A user semaphore already exists if the calling process is the owner
 * and the address of the stored user semaphore matches with the given one.
 *
 * @param sem The user semaphore.
 *
 * @return The result of the check.
 */
int usr_semaphore_sem_exist(usr_sem_t *sem)
{
  return (*sem < CONFIG_MAX_SEMAPHORES)
    && (usr_semaphore_table[*sem].owner_pid == process_get_current_process()
	&& usr_semaphore_table[*sem].sem_addr == (uint32_t)sem);
}

/**
 * Initializes a user semaphore at a given user space address.
 * The actual initialization is done by mapping a kernel semaphore
 * to the user semaphore.
 *
 * @param sem The user space semaphore.
 * @param val The initial value of the user semaphore.
 *
 * @return 0 on success and a negative value otherwise.
 *
 * @see semaphore_create
 * @see definition of return codes in usr_semaphore.h
 */
int usr_semaphore_create(usr_sem_t *sem, int val)
{
  interrupt_status_t intr_status;
  semaphore_t *k_semaphore;
  process_id_t pid;
  int index;

  /* Make sure val is not negative. */
  if (val < 0)
    return USR_SEMAPHORE_ERROR_VAL_NEGATIVE;

  /* Make sure the semaphore doesn't already exist. */
  if (usr_semaphore_sem_exist(sem)) {
    return USR_SEMAPHORE_ERROR_SEM_ALREADY_EXISTS;
  }
  
  k_semaphore = semaphore_create(val);

  /* Make sure a kernel semaphore could be created. */
  if (k_semaphore == NULL)
    return USR_SEMAPHORE_ERROR_KERNEL_SEM_NULL;
  
  /* Disable interrupts and acquire the user semaphore table spinlock. */
  intr_status = _interrupt_disable();
  spinlock_acquire(&usr_semaphore_slock);

  pid = process_get_current_process();  
  
  /* Find an empty spot in the user semaphore table. */
  index = usr_semaphore_find_empty_spot();

  if (index < 0) {
    /* Release the spinlock and restore interrupt status state. */
    spinlock_release(&usr_semaphore_slock);
    _interrupt_set_state(intr_status);
    return USR_SEMAPHORE_ERROR_NO_FREE_SEM;
  }

  /* Store the index of the table entry into the user space semaphore. */
  *sem = index;

  /* Do the actual mapping by storing the information in the table. */
  usr_semaphore_table[index].owner_pid = pid;
  usr_semaphore_table[index].sem_addr = (uint32_t)sem;
  usr_semaphore_table[index].kernel_sem = k_semaphore;

  /* Release the spinlock and restore interrupt status state. */
  spinlock_release(&usr_semaphore_slock);
  _interrupt_set_state(intr_status);

  return USR_SEMAPHORE_SUCCES;
}

/**
 * Decreases the value of a given user semaphore by one. If the semaphore
 * had a value of 0, then the call is blocked until someone else calls
 * usr_semaphore_V on the same user semaphore.
 *
 * @param sem The given user semaphore.
 *
 * @return 0 if the call succeded, negative if any errors occured.
 *
 * @see semaphore_P
 */
int usr_semaphore_P(usr_sem_t *sem)
{
  if (usr_semaphore_sem_exist(sem)) {
    semaphore_P(usr_semaphore_table[*sem].kernel_sem);
    return USR_SEMAPHORE_SUCCES;
  }
  return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
}

/**
 * Increases the value of a given user semaphore by one, and wakes up
 * one waiting thread if needed.
 *
 * @param sem The given user semaphore.
 *
 * @return 0 if the call succeded, negative if any errors occured.
 *
 * @see semaphore_V
 */
int usr_semaphore_V(usr_sem_t *sem)
{
  if (usr_semaphore_sem_exist(sem)) {
    semaphore_V(usr_semaphore_table[*sem].kernel_sem);
    return USR_SEMAPHORE_SUCCES;
  }
  return USR_SEMAPHORE_ERROR_SEM_DOES_NOT_EXIST;
}
