// syscall_showcase.c
// Demonstrates all 5 custom system calls in xv6.
// Run inside xv6 shell: $ syscall_showcase

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// ---- Worker function (used by thread_create demo) ----
void worker_func(void *param) {
  int num = (int)(uint64)param;
  printf("  [Worker] Running thread! value = %d\n", num);
  printf("  [Worker] PID = %d\n", getpid());
  exit(0);
}

// ---- Shared lock and value for mutex demo ----
int global_lock = 0;
int global_value = 0;

void lock_worker(void *param) {
  for(int j = 0; j < 5; j++){
    mutex_lock(&global_lock);
    global_value++;
    printf("  [Worker %d] value = %d\n", (int)(uint64)param, global_value);
    mutex_unlock(&global_lock);
  }
  exit(0);
}

// Prevent optimizer removal
void (*volatile keep_worker)(void*) = worker_func;

// -------------------------------------------------------
int main(void) {
  printf("\n=== xv6 Syscall Showcase ===\n\n");

  // --------------------------------------------------
  // DEMO 1: getpinfo — process details
  // --------------------------------------------------
  printf("--- Demo 1: getpinfo (Process Details) ---\n");
  struct pinfo pdata;
  int current_pid = getpid();

  if(getpinfo(current_pid, &pdata) == 0){
    printf("  PID:   %d\n", pdata.pid);
    printf("  PPID:  %d\n", pdata.ppid);
    printf("  State: %d (3=RUNNING)\n", pdata.state);
    printf("  Name:  %s\n", pdata.name);
  } else {
    printf("  getpinfo error!\n");
  }
  printf("\n");

  // --------------------------------------------------
  // DEMO 2: pipe2 — IPC
  // --------------------------------------------------
  printf("--- Demo 2: pipe2 (Inter-Process Communication) ---\n");
  int pipefd[2];

  if(pipe2(pipefd, 0) < 0){
    printf("  pipe2 error!\n");
  } else {
    printf("  Pipe created: read=%d write=%d\n", pipefd[0], pipefd[1]);

    int child = fork();
    if(child == 0){
      // Child writes
      close(pipefd[0]);
      char text[] = "Message from pipe2!";
      write(pipefd[1], text, sizeof(text));
      close(pipefd[1]);
      exit(0);
    } else {
      // Parent reads
      close(pipefd[1]);
      char buffer[32];
      int bytes = read(pipefd[0], buffer, sizeof(buffer));
      if(bytes > 0)
        printf("  Parent got: \"%s\"\n", buffer);

      close(pipefd[0]);
      wait(0);
    }
  }
  printf("\n");

  // --------------------------------------------------
  // DEMO 3: thread_create — threads
  // --------------------------------------------------
  printf("--- Demo 3: thread_create (Thread Demo) ---\n");

  char *stack_mem = malloc(4096 * 2);
  if(stack_mem == 0){
    printf("  stack allocation failed!\n");
  } else {
    char *aligned = (char*)(((uint64)stack_mem + 4095) & ~4095);
    void *top = aligned + 4096;

    void (*func_ptr)(void*) = worker_func;
    printf("  func=%p stack_top=%p\n", func_ptr, top);

    int thread_id = thread_create(func_ptr, (void*)99, top);
    if(thread_id < 0){
      printf("  thread_create error!\n");
    } else {
      printf("  Thread started TID=%d\n", thread_id);
      wait(0);
      printf("  Thread completed.\n");
    }

    free(stack_mem);
  }
  printf("\n");

  // --------------------------------------------------
  // DEMO 4: mutex_lock / unlock — synchronization
  // --------------------------------------------------
  printf("--- Demo 4: mutex (Lock Demo) ---\n");

  global_lock = 0;
  global_value = 0;

  char *mem1 = malloc(4096 * 2);
  char *mem2 = malloc(4096 * 2);

  if(mem1 && mem2){
    char *align1 = (char*)(((uint64)mem1 + 4095) & ~4095);
    char *align2 = (char*)(((uint64)mem2 + 4095) & ~4095);

    int id1 = thread_create(lock_worker, (void*)1, align1 + 4096);
    printf("  Started worker TID=%d\n", id1);
    wait(0);

    int id2 = thread_create(lock_worker, (void*)2, align2 + 4096);
    printf("  Started worker TID=%d\n", id2);
    wait(0);

    printf("  Final value = %d\n", global_value);
  }

  if(mem1) free(mem1);
  if(mem2) free(mem2);

  printf("\n");
  return 0;
}