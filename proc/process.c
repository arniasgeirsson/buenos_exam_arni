/*
 * Process startup.
 *
 * Copyright (C) 2003-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
 *       Teemu Takanen, Aleksi Virtanen, Troels Henriksen.
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
 * $Id: process.c,v 1.11 2007/03/07 18:12:00 ttakanen Exp $
 *
 */

#include "proc/process.h"
#include "proc/syscall.h"
#include "proc/elf.h"
#include "kernel/thread.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "kernel/config.h"
#include "kernel/sleepq.h"
#include "fs/vfs.h"
#include "drivers/yams.h"
#include "vm/vm.h"
#include "vm/pagepool.h"
#include "proc/exit.h"
#include "proc/usr_semaphore.h"
#include "lib/debug.h"

/** @name Process startup
 *
 * This module contains a function to start a userland process.
 */

process_table_t process_table[PROCESS_MAX_PROCESSES];

spinlock_t process_table_slock;

void process_init()
{
    int i;
    spinlock_reset(&process_table_slock);
    for (i = 0; i <= PROCESS_MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_FREE;
        process_table[i].executable[0] = 0;
    }
}

process_id_t alloc_process_id(process_state_t newstate)
{
    int i;
    interrupt_status_t intr_status;
      
    intr_status = _interrupt_disable();

    spinlock_acquire(&process_table_slock);
    for (i = 0; i <= PROCESS_MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_FREE) {
            process_table[i].state = newstate;
            break;
        }
    }
    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);
    return i <= PROCESS_MAX_PROCESSES ? i : -1;
}


/**
 * Starts one userland process. The thread calling this function will
 * be used to run the process and will therefore never return from
 * this function. This function asserts that no errors occur in
 * process startup (the executable file exists and is a valid ecoff
 * file, enough memory is available, file operations succeed...).
 * Therefore this function is not suitable to allow startup of
 * arbitrary processes.
 *
 * @executable The name of the executable to be run in the userland
 * process
 */
void process_start(uint32_t pid)
{
    thread_table_t *my_entry;
    pagetable_t *pagetable;
    uint32_t phys_page;
    context_t user_context;
    uint32_t stack_bottom;
    elf_info_t elf;
    openfile_t file;
    const char* executable;

    int i;

    interrupt_status_t intr_status;

    uint32_t *exitcode_phys;
    uint32_t *exitcode_static;

    my_entry = thread_get_current_thread_entry();
    my_entry->process_id = pid;
    executable = process_table[pid].executable;

    /* If the pagetable of this thread is not NULL, we are trying to
       run a userland process for a second time in the same thread.
       This is not possible. */
    KERNEL_ASSERT(my_entry->pagetable == NULL);

    pagetable = vm_create_pagetable(pid);
    KERNEL_ASSERT(pagetable != NULL);

    intr_status = _interrupt_disable();
    my_entry->pagetable = pagetable;
    _interrupt_set_state(intr_status);

    file = vfs_open((char *)executable);
    /* Make sure the file existed and was a valid ELF file */
    KERNEL_ASSERT(file >= 0);
    KERNEL_ASSERT(elf_parse_header(&elf, file));

    /* Trivial and naive sanity check for entry point: */
    KERNEL_ASSERT(elf.entry_point >= PAGE_SIZE);

    /* Allocate and map stack */
    for(i = 0; i < CONFIG_USERLAND_STACK_SIZE; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page, 
               (USERLAND_STACK_TOP & PAGE_SIZE_MASK) - i*PAGE_SIZE, 1);
    }

    /* Allocate and map pages for the segments. We assume that
       segments begin at page boundary. (The linker script in tests
       directory creates this kind of segments) */
    for(i = 0; i < (int)elf.ro_pages; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page, 
               elf.ro_vaddr + i*PAGE_SIZE, 1);
    }
    /* remember last physical page of RO segment to copy in exit code later */
    process_table[pid].exitfn 
      = elf.ro_vaddr + elf.ro_pages*PAGE_SIZE - 3*sizeof(uint32_t);
    exitcode_phys = (uint32_t*) 
      ADDR_PHYS_TO_KERNEL(phys_page + PAGE_SIZE - 3*sizeof(uint32_t));

    for(i = 0; i < (int)elf.rw_pages; i++) {
        phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(my_entry->pagetable, phys_page, 
               elf.rw_vaddr + i*PAGE_SIZE, 1);
    }

    /* Initialize heap pointer */
    uint32_t heap_end = elf.rw_vaddr+elf.rw_size;
    process_table[pid].heap_end = heap_end;
    if(heap_end % PAGE_SIZE == 0) {
        /* In the unlikely event that the heap should start on the 
           first address of a page we must allocate that page. */
        uint32_t phys_page = pagepool_get_phys_page();
        KERNEL_ASSERT(phys_page != 0);
        vm_map(pagetable, phys_page, heap_end, 1);   
    }

    /* Update TLB to allow access to the virtual addresses of the 
       segments. */
    _tlb_set_asid(thread_get_current_thread());

    /* Zero the pages. */
    memoryset((void *)elf.ro_vaddr, 0, elf.ro_pages*PAGE_SIZE);
    memoryset((void *)elf.rw_vaddr, 0, elf.rw_pages*PAGE_SIZE);

    stack_bottom = (USERLAND_STACK_TOP & PAGE_SIZE_MASK) - 
        (CONFIG_USERLAND_STACK_SIZE-1)*PAGE_SIZE;
    memoryset((void *)stack_bottom, 0, CONFIG_USERLAND_STACK_SIZE*PAGE_SIZE);

    /* Copy segments */

    if (elf.ro_size > 0) {
	/* Make sure that the segment is in proper place. */
        KERNEL_ASSERT(elf.ro_vaddr >= PAGE_SIZE);
        KERNEL_ASSERT(vfs_seek(file, elf.ro_location) == VFS_OK);
        KERNEL_ASSERT(vfs_read(file, (void *)elf.ro_vaddr, elf.ro_size)
		      == (int)elf.ro_size);
    }

    /* at the end of the last RO page, copy in 12 bytes code for
       syscall exit (called by forked threads when they finish) */
    exitcode_static = (uint32_t*) do_syscall_exit;
    exitcode_phys[0] = (uint32_t) exitcode_static[0];
    exitcode_phys[1] = (uint32_t) exitcode_static[1];
    exitcode_phys[2] = (uint32_t) exitcode_static[2];

    if (elf.rw_size > 0) {
	/* Make sure that the segment is in proper place. */
        KERNEL_ASSERT(elf.rw_vaddr >= PAGE_SIZE);
        KERNEL_ASSERT(vfs_seek(file, elf.rw_location) == VFS_OK);
        KERNEL_ASSERT(vfs_read(file, (void *)elf.rw_vaddr, elf.rw_size)
		      == (int)elf.rw_size);
    }


    /* Set the dirty bit to zero (read-only) on read-only pages. */
    for(i = 0; i < (int)elf.ro_pages; i++) {
        vm_set_dirty(my_entry->pagetable, elf.ro_vaddr + i*PAGE_SIZE, 0);
    }

    _tlb_set_asid(thread_get_current_thread());

    /* Initialize the user context. (Status register is handled by
       thread_goto_userland) */
    memoryset(&user_context, 0, sizeof(user_context));
    user_context.cpu_regs[MIPS_REGISTER_SP] = USERLAND_STACK_TOP;
    user_context.pc = elf.entry_point;

    vfs_close(file);

    thread_goto_userland(&user_context);

    KERNEL_PANIC("thread_goto_userland failed.");
}


process_id_t process_spawn(const char* executable)
{
    TID_t thread;
    interrupt_status_t intr_status;
    device_t *dev;
    process_id_t my_pid = process_get_current_process();
    process_id_t pid = alloc_process_id(PROCESS_RUNNING);

    if (pid < 0) { /* Process table full */
        return -1;
    }
    stringcopy(process_table[pid].executable, executable, PROCESS_NAME_MAX);
    process_table[pid].retval = 0;
    process_table[pid].first_zombie = -1;
    process_table[pid].prev_zombie = -1;
    process_table[pid].next_zombie = -1;
    process_table[pid].parent = my_pid;

    intr_status = _interrupt_disable();
    spinlock_acquire(&process_table_slock);

    if (my_pid >= 0) {
        process_table[my_pid].children++;
    }

    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);

    process_table[pid].children = 0;
    process_table[pid].threads = 1;
    process_table[pid].stack_end =
        (USERLAND_STACK_TOP & PAGE_SIZE_MASK)
        - CONFIG_USERLAND_STACK_SIZE*PAGE_SIZE;
    process_table[pid].bot_free_stack = 0;
    dev = device_get(YAMS_TYPECODE_TTY, 0);
    KERNEL_ASSERT(dev != NULL);
    KERNEL_ASSERT(dev->generic_device != NULL);
    process_table[pid].fds[0] =
        process_table[pid].fds[1] = (gcd_t *)dev->generic_device;
    thread = thread_create((void (*)(uint32_t))(&process_start), (uint32_t)pid);
    thread_run(thread);
    kprintf("Spawned process %d, parent %d\n",pid,my_pid);
    return pid;
}


process_id_t process_get_current_process(void)
{
    return thread_get_current_thread_entry()->process_id;
}

process_table_t *process_get_current_process_entry(void)
{
    return &process_table[process_get_current_process()];
}

uint32_t process_join(process_id_t pid)
{
    process_id_t my_pid;
    uint32_t retval;
    interrupt_status_t intr_status;
  
    my_pid = process_get_current_process();
    if (pid < 0
        || pid >= PROCESS_MAX_PROCESSES
        || process_table[pid].parent != my_pid) {
        return -1;
    }

    intr_status = _interrupt_disable();
    spinlock_acquire(&process_table_slock);

    while (process_table[pid].state != PROCESS_ZOMBIE) {
        sleepq_add(&process_table[pid]);
        spinlock_release(&process_table_slock);
        thread_switch();
        spinlock_acquire(&process_table_slock);
    }
    retval = process_table[pid].retval;

    kprintf("Woke after join on %d, i am %d\n",pid,my_pid); 
    // process_table[my_pid].children--;
    
    

    /* Let children see it is gone. */
    process_table[pid].retval = -1;
    /* Make sure we can't join it again. */
    process_table[pid].parent = -1;

    if (process_table[pid].children == 0) {
      process_table[pid].state = PROCESS_FREE;
      /* ............ */
      process_table[my_pid].children--;
      if (process_table[my_pid].first_zombie == pid) {
	process_id_t next = process_table[pid].next_zombie;
	process_table[my_pid].first_zombie = next;
	if (next >= 0) {
	  process_table[next].prev_zombie = -1;
	}
      } else {
	process_id_t prev = process_table[pid].prev_zombie;
	process_id_t next = process_table[pid].next_zombie;
	process_table[prev].next_zombie = next;
	if (next >= 0) {
	  process_table[next].prev_zombie = prev;
	}
      }
      /* ------------- */
    }
    
    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);
    return retval;
}


/**
 * This function inserts the userspace thread stack in a list of free
 * stacks maintained in the process table entry.  This means that
 * when/if the next thread is created, we can reuse one of the old
 * stacks, and reduce memory usage.  Note that the stack is not really
 * "deallocated" per se, and still counts towards the 64KiB memory
 * limit for processes.  This is a simple mechanism, not a very good
 * one.  This function assumes that the process table is already
 * locked.
 * 
 * @param my_thread The thread whose stack should be deallocated.
 *
 */
void process_free_stack(thread_table_t *my_thread)
{
    /* Assume we have lock on the process table. */
    process_id_t my_pid = my_thread->process_id;
    uint32_t old_free_list = process_table[my_pid].bot_free_stack;
    /* Find the stack by applying a mask to the stack pointer. */
    uint32_t stack =
        (my_thread->user_context->cpu_regs[MIPS_REGISTER_SP]
         & USERLAND_STACK_MASK)
        + CONFIG_USERLAND_STACK_SIZE*PAGE_SIZE
        - 4;

    KERNEL_ASSERT(stack >= process_table[my_pid].stack_end);

    process_table[my_pid].bot_free_stack = stack;
    *(uint32_t*)stack = old_free_list;
}


void finish_given_process(process_id_t pid, int retval)
{
    process_id_t parent = process_table[pid].parent;
    process_id_t zombie;

    process_table[pid].retval = retval;

    while ((zombie = process_table[pid].first_zombie) >= 0) {
        /* We have zombie children - remove them. */
        process_table[zombie].state = PROCESS_FREE;
        process_table[zombie].retval = -1;
        process_table[zombie].parent = -1;
        process_table[pid].first_zombie = process_table[zombie].next_zombie;
        process_table[pid].children--;
	//kprintf("bbb zombie = %d, pid = %d, first zombie: %d, next zombie: %d children: %d\n",zombie, pid,process_table[pid].first_zombie,process_table[zombie].next_zombie,process_table[pid].children);
    }

    if (parent >= 0
        && process_table[parent].state == PROCESS_ZOMBIE) {
        /* We have a zombie parent, implying we can never be
           joined. */
        if (--(process_table[parent].children) == 0
            && process_table[parent].retval < 0) {
            /* Oh, and our parent is joined and we are the last
               child. so free our parent. */
            finish_given_process(parent, 0);
        }
        process_table[pid].state = PROCESS_FREE;
    } else if (parent >= 0) {
        /* Our parent is alive and well, add us to its list of zombies */
      /* kprintf("%d Zombie list before:\n",pid);
      process_id_t k = process_table[parent].first_zombie;
      int c = 0;
      while (k >= 0) {
	kprintf("%d <- %d -> %d\n",process_table[k].prev_zombie,k,process_table[k].next_zombie);
	k = process_table[k].next_zombie;
	c++;
	if (c == 3) break;
	}*/

        process_table[pid].state = PROCESS_ZOMBIE;
        zombie = process_table[parent].first_zombie;
        process_table[pid].next_zombie = zombie;
        if (zombie >= 0) {
            process_table[zombie].prev_zombie = pid;
        }
        process_table[parent].first_zombie = pid;

	/*
	kprintf("%d Zombie list after:\n",pid);
      k = process_table[parent].first_zombie;
      c = 0;
      while (k >= 0) {
	kprintf("%d <- %d -> %d\n",process_table[k].prev_zombie,k,process_table[k].next_zombie);
	k = process_table[k].next_zombie;
	c++;
	if (c==3) break;
	}*/
    } else {
        /* We have no parent, i.e. we are the initial program, and
           will be joined by the startup thread in init/main.c */
        process_table[pid].state = PROCESS_ZOMBIE;
    }

    sleepq_wake(&process_table[pid]);
}


/**
 *
 * Terminate the current process (maybe).  If the current process has
 * more than one running thread, only terminate the current thread.
 * The process is only completely terminated (as per process_join
 * wakeup and page table deallocation) when the final thread calls
 * process_finish().
 *
 * @param The return value of the process.  This is only used when the
 * final thread exits.
 *
 */
void process_finish(int retval)
{
    interrupt_status_t intr_status;
    thread_table_t *thread = thread_get_current_thread_entry();
    process_id_t pid = thread->process_id;

    if (retval < 0) {
        /* Not permitted! */
        retval = 0;
    }

    intr_status = _interrupt_disable();
    spinlock_acquire(&process_table_slock);

    /* Mark the stack as free so new threads can reuse it. */
    process_free_stack(thread);

    if (--process_table[pid].threads == 0) {
        /* We are the last thread - kill process! */
	/* ---- */
	usr_semaphore_process_died(pid);
	/* ----- */
        vm_destroy_pagetable(thread->pagetable);
        finish_given_process(pid, retval);
    }

    thread->pagetable = NULL;

    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);
    thread_finish();
}

/**
 * We need to pass a bunch of data to the new thread, but we can only
 * pass a single 32 bit number!  How do we deal with that?  Simple -
 * we allocate a structure on the stack of the forking kernel thread
 * containing all the data we need, with a 'done' field that indicates
 * when the new thread has copied over the data.  See process_fork().
 */
typedef struct thread_params_t {
    volatile uint32_t done; /* Don't cache in register. */
    void (*func)(uint32_t);
    uint32_t arg;
    process_id_t pid;
    pagetable_t *pagetable;
} thread_params_t;


void setup_thread(thread_params_t *params)
{
    context_t user_context;
    uint32_t phys_page;
    int i;
    interrupt_status_t intr_status;
    thread_table_t *thread= thread_get_current_thread_entry();

    /* Copy thread parameters. */
    int arg = params->arg;
    void (*func)(uint32_t) = params->func;
    process_id_t pid = thread->process_id = params->pid;
    thread->pagetable = params->pagetable;
    params->done = 1; /* OK, we don't need params any more. */

    intr_status = _interrupt_disable();
    spinlock_acquire(&process_table_slock);

    /* Set up userspace environment. */
    memoryset(&user_context, 0, sizeof(user_context));
    
    user_context.cpu_regs[MIPS_REGISTER_A0] = arg;
    user_context.pc = (uint32_t)func;
   
    /* Allocate thread stack */
    if (process_table[pid].bot_free_stack != 0) {
        /* Reuse old thread stack. */
        user_context.cpu_regs[MIPS_REGISTER_SP] =
            process_table[pid].bot_free_stack;
        process_table[pid].bot_free_stack =
            *(uint32_t*)process_table[pid].bot_free_stack;
    } else {
        /* Allocate physical pages (frames) for the stack. */
        for (i = 0; i < CONFIG_USERLAND_STACK_SIZE; i++) {
            phys_page = pagepool_get_phys_page();
            KERNEL_ASSERT(phys_page != 0);
            vm_map(thread->pagetable, phys_page, 
                   process_table[pid].stack_end - (i+1)*PAGE_SIZE, 1);
        }
        user_context.cpu_regs[MIPS_REGISTER_SP] =
            process_table[pid].stack_end-4; /* Space for the thread argument */
        process_table[pid].stack_end -= PAGE_SIZE*CONFIG_USERLAND_STACK_SIZE;
    }

    /* set exiting function address */
    user_context.cpu_regs[MIPS_REGISTER_RA]= process_table[pid].exitfn;

    _tlb_set_asid(thread_get_current_thread());

    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);

    thread_goto_userland(&user_context);
}

int process_fork(void (*func)(uint32_t), uint32_t arg)
{
    TID_t tid;
    thread_table_t *thread = thread_get_current_thread_entry();
    process_id_t pid = thread->process_id;
    interrupt_status_t intr_status;
    thread_params_t params;
    params.done = 0;
    params.func = func;
    params.arg = arg;
    params.pid = pid;
    params.pagetable = thread->pagetable;

    intr_status = _interrupt_disable();
    spinlock_acquire(&process_table_slock);

    tid = thread_create((void (*)(uint32_t))(setup_thread), (uint32_t)&params);

    if (tid < 0) {
        spinlock_release(&process_table_slock);
        _interrupt_set_state(intr_status);
        return -1;
    }

    process_table[pid].threads++;

    spinlock_release(&process_table_slock);
    _interrupt_set_state(intr_status);
    
    thread_run(tid);

    /* params will be dellocated when we return, so don't until the
       new thread is ready. */
    while (!params.done);

    return tid;
}

/** @} */
