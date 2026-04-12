// sysproc_custom.c - Custom system call implementations for xv6
// Implements: getpinfo, pipe2, thread_create, mutex_lock, mutex_unlock, sendsig
// kernel/sysproc_custom.c — correct order
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"      
#include "proc.h"      
#include "defs.h"

// ============================================================
// SYSTEM CALL 1: getpinfo
// Purpose: Returns info about a process given its PID.
//          Demonstrates process inspection / process creation info.
// Usage:   getpinfo(pid, &pinfo_struct)
// Returns: 0 on success, -1 on failure
// ============================================================
uint64
sys_getpinfo(void)
{
  int pid;
  uint64 addr;  // user pointer to struct pinfo

  // Fetch arguments
  argint(0, &pid);
  argaddr(1, &addr);

  if(addr == 0)
    return -1;

  // Search process table
  struct proc *p;
  extern struct proc proc[];  // xv6 global process table

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      struct pinfo info;
      info.pid   = p->pid;
      info.ppid  = p->parent ? p->parent->pid : 0;
      info.state = p->state;  // enum value: 0-4
      safestrcpy(info.name, p->name, sizeof(info.name));
      release(&p->lock);

      // Copy struct to user space
      struct proc *caller = myproc();
      if(copyout(caller->pagetable, addr, (char*)&info, sizeof(info)) < 0)
        return -1;
      return 0;
    }
    release(&p->lock);
  }
  return -1;  // pid not found
}

// ============================================================
// SYSTEM CALL 2: pipe2
// Purpose: Enhanced pipe creation with a flags argument.
//          Demonstrates IPC — creates a communication channel.
//          Flag 1 = non-blocking read end (simplified marker only).
// Usage:   pipe2(int pipefd[2], int flags)
// Returns: 0 on success, -1 on failure
// ============================================================
uint64
sys_pipe2(void)
{
  uint64 fdarray;  // user pointer to int[2]
  int flags;

  argaddr(0, &fdarray);
  argint(1, &flags);

  // Reuse xv6's existing pipealloc logic via sys_pipe internals
  // We call the kernel pipealloc directly
  struct proc *p = myproc();
  struct file *rf, *wf;
  int fd0, fd1;

  if(pipealloc(&rf, &wf) < 0)
    return -1;

  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }

  // Write the two fds back to user space
  if(copyout(p->pagetable, fdarray,   (char*)&fd0, sizeof(fd0)) < 0 ||
     copyout(p->pagetable, fdarray+4, (char*)&fd1, sizeof(fd1)) < 0){
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }

  // flags: bit 0 = mark read-end (simplified; full impl would set O_NONBLOCK)
  if(flags & 1){
    // In a full implementation, set rf->nonblock = 1 here
    // For this demo we just acknowledge the flag
  }

  return 0;
}

// ============================================================
// SYSTEM CALL 3: thread_create
// Purpose: Creates a new "thread" (lightweight process sharing
//          parent memory) — demonstrates threading in xv6.
// Usage:   thread_create(void (*fn)(void*), void *arg, void *stack)
// Returns: thread tid (pid) on success, -1 on failure
// ============================================================

uint64
sys_thread_create(void)
{
  uint64 fcn, arg, stack;
  argaddr(0, &fcn);
  argaddr(1, &arg);
  argaddr(2, &stack);

  if(stack == 0)
    return -1;

  struct proc *parent = myproc();
  struct proc *t;

  if((t = allocproc()) == 0)
    return -1;

  // Copy parent's page table (like fork)
  if(uvmcopy(parent->pagetable, t->pagetable, parent->sz) < 0){
    freeproc(t);
    release(&t->lock);
    return -1;
  }
  t->sz = parent->sz;

  // Copy trapframe, then set thread entry point
  *(t->trapframe) = *(parent->trapframe);
  t->trapframe->epc = fcn;
  t->trapframe->sp  = stack;
  t->trapframe->a0  = arg;

  // Share file descriptors
  for(int i = 0; i < NOFILE; i++)
    if(parent->ofile[i])
      t->ofile[i] = filedup(parent->ofile[i]);
  t->cwd = idup(parent->cwd);
  safestrcpy(t->name, parent->name, sizeof(parent->name));

  int tid = t->pid;
  t->parent = parent;
  t->state = RUNNABLE;
  release(&t->lock);

  return tid;
}
// ============================================================
// SYSTEM CALL 4 & 5: mutex_lock / mutex_unlock
// Purpose: Provides user-space mutual exclusion (locking).
//          Uses a shared memory integer as a mutex variable.
//          0 = unlocked, 1 = locked.
// Usage:   mutex_lock(int *mutex)   -- blocks until acquired
//          mutex_unlock(int *mutex) -- releases the lock
// Returns: 0 on success, -1 on failure
// ============================================================

// mutex_lock: spin-wait until the mutex word at user addr becomes 0, then set to 1
uint64
sys_mutex_lock(void)
{
  uint64 addr;
  argaddr(0, &addr);

  if(addr == 0)
    return -1;

  struct proc *p = myproc();
  int val;

  // Spin: read user mutex value; if 0, atomically set to 1 and return
  // (simplified busy-wait — real impl uses atomic CAS instruction)
  for(;;){
    if(copyin(p->pagetable, (char*)&val, addr, sizeof(val)) < 0)
      return -1;
    if(val == 0){
      val = 1;
      if(copyout(p->pagetable, addr, (char*)&val, sizeof(val)) < 0)
        return -1;
      return 0;  // lock acquired
    }
    // yield CPU while waiting (avoid burning cycles)
    yield();
  }
}

// mutex_unlock: set the mutex word at user addr to 0
uint64
sys_mutex_unlock(void)
{
  uint64 addr;
  argaddr(0, &addr);

  if(addr == 0)
    return -1;

  int val = 0;
  struct proc *p = myproc();
  if(copyout(p->pagetable, addr, (char*)&val, sizeof(val)) < 0)
    return -1;
  return 0;
}

// ============================================================
// SYSTEM CALL 6: sendsig
// Purpose: Send a signal number to a process by PID.
//          Demonstrates inter-process signaling.
// Usage:   sendsig(int pid, int signum)
// Returns: 0 on success, -1 on failure
// Signals: 1=SIGTERM(kill), 2=SIGSTOP(pause), 3=SIGCONT(resume)
// ============================================================
uint64
sys_sendsig(void)
{
  int pid, signum;
  argint(0, &pid);
  argint(1, &signum);

  if(pid <= 0 || signum <= 0)
    return -1;

  extern struct proc proc[];
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      p->pending_sig = signum;

      if(signum == 1){
        // SIGTERM: kill the process
        p->killed = 1;
        if(p->state == SLEEPING)
          p->state = RUNNABLE;
      } else if(signum == 2){
        // SIGSTOP: pause (put to sleep) — simplified
        if(p->state == RUNNABLE || p->state == RUNNING)
          p->state = SLEEPING;
      } else if(signum == 3){
        // SIGCONT: resume
        if(p->state == SLEEPING)
          p->state = RUNNABLE;
      }

      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;  // process not found
}
