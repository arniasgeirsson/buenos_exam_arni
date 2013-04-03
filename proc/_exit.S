/* _exit.S: code to exit forked user threads, copied into new threads
 */

#include "lib/registers.h"
#include "proc/syscall.h"

        .text
	.align	2

/* make a syscall_exit from userland.
 * These assembler instructions will be copied into a forked thread's
 * stack space.
 */

# void do_syscall_exit(void)
        .globl  do_syscall_exit
	.ent	do_syscall_exit

do_syscall_exit:
        li      a0, SYSCALL_EXIT
        li      a1, 0
        syscall
        .end    do_syscall_exit
