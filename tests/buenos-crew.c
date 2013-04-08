#include "tests/lib.h"

usr_sem_t rmutex, wmutex, writing, reading;

int readcount=0, writecount=0;


void writer(int id) {
  while (1) {
    // producing data
    syscall_sem_p(&wmutex);
    if (writecount == 0) { syscall_sem_p(&reading); }
    writecount += 1;
    syscall_sem_v(&wmutex);
    syscall_sem_p(&writing);
    // writing data
    syscall_sem_p(&wmutex);
    writecount -= 1;
    printf("Writer %d wrote\n", id);
    if (writecount == 0) { syscall_sem_v(&reading); }
    syscall_sem_v(&wmutex);
    syscall_sem_v(&writing);
  }
}

void reader(int id) {
  while (1) {
    syscall_sem_p(&reading);
    syscall_sem_p(&rmutex);
    if (readcount == 0) { syscall_sem_p(&writing); }
    readcount += 1;
    syscall_sem_v(&rmutex);
    syscall_sem_v(&reading);
    // reading data
    syscall_sem_p(&rmutex);
    readcount -= 1;
    printf("Reader %d read\n", id);
    if (readcount == 0) { syscall_sem_v(&writing); }
    syscall_sem_v(&rmutex);
  }
}

int main() {
  int readers = 1, writers = 1;
  int i;
  printf("create 1\n");
  syscall_sem_create(&rmutex, 1);
  printf("create 2\n");
  syscall_sem_create(&wmutex, 1);
  printf("create 3\n");
  syscall_sem_create(&reading, 1);
  printf("create 4\n");
  syscall_sem_create(&writing, 1);
  for (i = 0; i < readers; i++) {
    syscall_fork(reader, i);
  }
  for (i = 0; i < writers; i++) {
    syscall_fork(writer, i);
  }
  return 0;
}
