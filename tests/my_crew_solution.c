#include "tests/lib.h"

usr_sem_t gate,rmutex, writing;

int readcount = 0;

void writer(int id) {
  while (1) {
    // producing data
    syscall_sem_p(&gate);
    syscall_sem_p(&writing);
    syscall_sem_v(&gate);
    // writing data
    printf("Writer %d writing.\n", id);
    syscall_sem_v(&writing);
  }
}

void reader(int id) {
  while (1) {
    syscall_sem_p(&gate);
    syscall_sem_p(&rmutex);
    readcount += 1;
    if (readcount == 1) { syscall_sem_p(&writing); }
    syscall_sem_v(&rmutex);
    syscall_sem_v(&gate);
    // reading data
    printf("Reader %d reading.\n", id);

    syscall_sem_p(&rmutex);
    readcount -= 1;
    if (readcount == 0) { syscall_sem_v(&writing); }
    syscall_sem_v(&rmutex);
  }
}

int main() {
  int readers = 10, writers = 10;
  int i;
  printf("create 1%s\n","");
  syscall_sem_create(&rmutex, 1);
  printf("create 2%s\n","");
  syscall_sem_create(&writing, 1);
  printf("create 3%s\n","");
  syscall_sem_create(&gate, 1);
  for (i = 0; i < readers; i++) {
    if (syscall_fork(reader, i) == -1) {
      printf("\nNot enough threads to support the simulation, halting\n");
      syscall_halt();
    }
  }
  for (i = 0; i < writers; i++) {
    if (syscall_fork(writer, i) == -1) {
      printf("\nNot enough threads to support the simulation, halting\n");
      syscall_halt();
    }  }
  return 0;
}
