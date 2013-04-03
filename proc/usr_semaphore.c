#include "proc/usr_semaphore.h"
#include "proc/process.h"
#include "kernel/semaphore.h"
#include "lib/debug.h"

/* Put your function definitions here. */

/* ------------------ */

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
  process_table_t *pcb = process_get_current_process_entry();
  
  
  // lock?
  int i;
  for (i = 0; i < 32; i++) {
    if (pcb->semaphore_table2[i].free == 1) {
      pcb->semaphore_table2[i].free = 0;
      break;
    }
  }
  
  if (i == 32)
    return -3;
  
  pcb->semaphore_table2[i].user_sem = (uint32_t) sem;
  pcb->semaphore_table2[i].kernel_sem = (uint32_t)semaphore;

  return 0;
}

int usr_semaphore_P(uint32_t *sem)
{
  // lock?
  process_table_t *pcb = process_get_current_process_entry();
  

  // lock?
  int i;
  for (i = 0; i < 32; i++) {
    if (pcb->semaphore_table2[i].free == 0
	&& pcb->semaphore_table2[i].user_sem ==(uint32_t) sem)
      {
	DEBUG("task1_debug","P : found match at index %d\n",i);
	break;
      }
  }
  
  if (i == 32) {
    DEBUG("task1_debug","P: i == 32\n");
    return -3; }

  semaphore_P((semaphore_t*)pcb->semaphore_table2[i].kernel_sem);
  return 0;
}

int usr_semaphore_V(uint32_t *sem)
{

  // lock? 
  process_table_t *pcb = process_get_current_process_entry();
  
  // lock
  int i;
  for (i = 0; i < 32; i++) {
    if (pcb->semaphore_table2[i].free == 0
	&& pcb->semaphore_table2[i].user_sem ==(uint32_t) sem)
      {
	DEBUG("task1_debug","V : found match at index %d\n",i);
	break;
      }
  }
  
  if (i == 32) {
    DEBUG("task1_debug","V: i == 32\n");
    return -3; }

  semaphore_V((semaphore_t*)pcb->semaphore_table2[i].kernel_sem);

  return 0;
}

/* ---------------- */
