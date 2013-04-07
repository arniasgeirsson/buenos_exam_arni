/*
 * System calls.
 *
 * Copyright (C) 2003 Juha Aatrokoski, Timo Lilja,
 *   Leena Salmela, Teemu Takanen, Aleksi Virtanen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: syscall.c,v 1.3 2004/01/13 11:10:05 ttakanen Exp $
 *
 */
#include "kernel/cswitch.h"
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"
#include "proc/process.h"
#include "drivers/device.h"
#include "drivers/gcd.h"
#include "fs/vfs.h"
#include "kernel/thread.h"
#include "proc/usr_semaphore.h"
#include "kernel/scheduler.h"
#include "kernel/sleep.h"
#include "drivers/metadev.h"
#include "kernel/interrupt.h"

/**
 * Yields the current thread by manually deschedule the thread.
 * Nothing is done if the ready queue in the scheduler is empty.
 *
 * @return 0 on success and -1 if the ready queue was empty.
 *
 * @see scheduler_is_ready_queue_empty
 */
int syscall_yield(void)
{
  interrupt_status_t intr_status = _interrupt_disable();
  
  /* If the ready queue is empty we assume we would be
     the next to run. */
  if (scheduler_is_ready_queue_empty()) {
    _interrupt_set_state(intr_status);
    return -1;
  }

  /* thread switch enables interrupts and restores them afterwards. */
  thread_switch();
  _interrupt_set_state(intr_status);

  return 0;
}

/**
 * Makes the calling thread go into sleep for at least the specified amount
 * of milliseconds.
 * 
 * @param msec The specified amount of milliseconds.
 *
 * @return 0 on success.
 * @return -1 if msec is not postive.
 * @return -2 if the thread slept for less than msec.
 *
 * @see sleep_thread_sleep
 */
int syscall_sleep(int msec)
{
  /* Make sure msec is positive. */
  if (msec <= 0)
    return -1;

  int start = rtc_get_msec();
  /* Go into sleep. */ 
  sleep_thread_sleep(msec);
  int end = rtc_get_msec();

  /* Make sure that the thread slept for at least the specified
     amount of time. */
  if (end-start < msec)
    return -2; /* thread_sleep has failed, although should not happen. */
  return 0;
}

/**
 * Handle system calls. Interrupts are enabled when this function is
 * called.
 *
 * @param user_context The userland context (CPU registers as they
 * where when system call instruction was called in userland)
 */
void syscall_handle(context_t *user_context)
{
  int A1 = user_context->cpu_regs[MIPS_REGISTER_A1];
  int A2 = user_context->cpu_regs[MIPS_REGISTER_A2];
  int A3 = user_context->cpu_regs[MIPS_REGISTER_A3];

#define V0 (user_context->cpu_regs[MIPS_REGISTER_V0])

  /* When a syscall is executed in userland, register a0 contains
   * the number of the syscall. Registers a1, a2 and a3 contain the
   * arguments of the syscall. The userland code expects that after
   * returning from the syscall instruction the return value of the
   * syscall is found in register v0. Before entering this function
   * the userland context has been saved to user_context and after
   * returning from this function the userland context will be
   * restored from user_context.*/
  switch(user_context->cpu_regs[MIPS_REGISTER_A0])
  {
  case SYSCALL_HALT:
    halt_kernel();
    break;
  case SYSCALL_EXIT:
    process_finish(A1);
    break;
  case SYSCALL_WRITE:
    V0 = vfs_write(A1, (void*)A2, A3);
    break;
  case SYSCALL_READ:
    V0 = vfs_read(A1, (char *)A2, A3);
    break;
  case SYSCALL_JOIN:
    V0 = process_join(A1);
    break;
  case SYSCALL_EXEC:
    V0 = process_spawn((char *)A1);
    break;
  case SYSCALL_FORK:
    V0 = process_fork((void(*)(uint32_t))A1, A2) >= 0 ? 0 : -1;
    break;
  case SYSCALL_SEM_CREATE:
    V0 = usr_semaphore_create((uint32_t*)A1,A2);
    break;
  case SYSCALL_SEM_P:
    V0 = usr_semaphore_P((uint32_t*)A1);
    break;
  case SYSCALL_SEM_V:
    V0 = usr_semaphore_V((uint32_t*)A1);
    break;
  case SYSCALL_YIELD:
    V0 = syscall_yield();
    break;
  case SYSCALL_SLEEP:
    V0 = syscall_sleep(A1);
    break;
  case SYSCALL_SYSTEM_TIME:
    /* Return the elapsed time since system startup in milliseconds. */
    V0 = rtc_get_msec();
    break;
  default:
    KERNEL_PANIC("Unhandled system call\n");
  }

  /* Move to next instruction after system call */
  user_context->pc += 4;

#undef V0

}
