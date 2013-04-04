#include "tests/lib.h"

usr_sem_t mutex, write, incoming;

int readcount=0;


void writer(int id) {
  while (1) {
    // producing data
    syscall_sem_p(&incoming);
    syscall_sem_p(&write);
    syscall_sem_v(&incoming);
    // writing data
    printf("Writer %d wrote\n", id);
    syscall_sem_v(&write);
  }
}

void reader(int id) {
  while (1) {
    syscall_sem_p(&incoming);
    syscall_sem_p(&mutex);
    if (++readcount == 1) { syscall_sem_p(&write); }
    syscall_sem_v(&mutex);
    syscall_sem_v(&incoming);
    // reading data
    printf("Reader %d read\n", id);

    syscall_sem_p(&mutex);
    if (--readcount == 0) { syscall_sem_v(&write); }
    syscall_sem_v(&mutex);
  }
}

int main() {
  int readers = 10, writers = 10;
  int i;
  printf("create 1%s\n","");
  syscall_sem_create(&mutex, 1);
  printf("create 2%s\n","");
  syscall_sem_create(&write, 1);
  printf("create 3%s\n","");
  syscall_sem_create(&incoming, 1);
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
