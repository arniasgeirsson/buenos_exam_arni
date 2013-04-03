#include "proc/usr_semaphore.h"
#include "proc/process.h"
#include "kernel/semaphore.h"
#include "lib/debug.h"
#include "kernel/assert.h"

/* Put your function definitions here. */

/* ------------------ */
void usr_semaphore_init(void)
{
  int u;
  for (u=0; u < CONFIG_MAX_SEMAPHORES; u++) {
    usr_semaphore_table[u].owner_pid = -1;
  }
}

void usr_semaphore_process_died(int pid)
{
  KERNEL_ASSERT(pid > -1 && pid < PROCESS_MAX_PROCESSES);
  int u;
  for (u=0; u < CONFIG_MAX_SEMAPHORES; u++) {
    if (usr_semaphore_table[u].owner_pid == pid) {
      semaphore_destroy((semaphore_t*)usr_semaphore_table[u].kernel_sem);
      usr_semaphore_table[u].owner_pid = -1;
    }
  }
}

int usr_semaphore_create(uint32_t *sem, int val)
{
  DEBUG("task1_debug","create\n");

  if (val < 0)
    return -1;

  sem = sem; /* check that sem is already in memory */
  
  semaphore_t *semaphore = semaphore_create(val);
  if (semaphore == NULL)
    return -2;
  DEBUG("task1_debug","kernel semaphore is %d\n",(int)semaphore);
  // lock?
  process_id_t pid = process_get_current_process();
  
  
  // lock?
  int i;
  for (i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == -1) {
      usr_semaphore_table[i].owner_pid = (int)pid;
      break;
    }
  }
  
  if (i == CONFIG_MAX_SEMAPHORES)
    return -3;
  
  usr_semaphore_table[i].user_sem = (uint32_t) sem;
  usr_semaphore_table[i].kernel_sem = (uint32_t)semaphore;

  return 0;
}

int usr_semaphore_P(uint32_t *sem)
{
  // lock?
  process_id_t pid = process_get_current_process();
  
  // lock?
  int i;
  for (i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == pid
	&& usr_semaphore_table[i].user_sem ==(uint32_t) sem)
      {
	DEBUG("task1_debug","P : found match at index %d\n",i);
	break;
      }
  }
  
  if (i == CONFIG_MAX_SEMAPHORES) {
    DEBUG("task1_debug","P: i == 32\n");
    return -3; }

  semaphore_P((semaphore_t*)usr_semaphore_table[i].kernel_sem);
  return 0;
}

int usr_semaphore_V(uint32_t *sem)
{

  // lock? 
  process_id_t pid = process_get_current_process();
  
  // lock
  int i;
  for (i = 0; i < CONFIG_MAX_SEMAPHORES; i++) {
    if (usr_semaphore_table[i].owner_pid == pid
	&& usr_semaphore_table[i].user_sem ==(uint32_t) sem)
      {
	DEBUG("task1_debug","V : found match at index %d\n",i);
	break;
      }
  }
  
  if (i == CONFIG_MAX_SEMAPHORES) {
    DEBUG("task1_debug","V: i == 32\n");
    return -3; }

  semaphore_V((semaphore_t*)usr_semaphore_table[i].kernel_sem);

  return 0;
}

/* ---------------- */
