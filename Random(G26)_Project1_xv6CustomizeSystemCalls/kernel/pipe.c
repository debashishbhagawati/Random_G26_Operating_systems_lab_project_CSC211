#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

/* ===================== PIPE STRUCT ===================== */

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;
  uint nwrite;
  int readopen;
  int writeopen;
};

/* ===================== PIPE ALLOCATION ===================== */

int pipealloc(struct file **f0, struct file **f1) {
  struct pipe *pi = 0;

  *f0 = *f1 = 0;

  if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;

  if ((pi = (struct pipe *)kalloc()) == 0)
    goto bad;

  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nread = 0;
  pi->nwrite = 0;

  initlock(&pi->lock, "pipe");

  /* setup read end */
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;

  /* setup write end */
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;

  return 0;

bad:
  if (pi)
    kfree((char *)pi);

  if (*f0)
    fileclose(*f0);

  if (*f1)
    fileclose(*f1);

  return -1;
}

/* ===================== PIPE CLOSE ===================== */

void pipeclose(struct pipe *pi, int writable) {
  acquire(&pi->lock);

  if (writable) {
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }

  int should_free = (pi->readopen == 0 && pi->writeopen == 0);

  release(&pi->lock);

  if (should_free)
    kfree((char *)pi);
}

/* ===================== WRITE ===================== */

static int pipe_can_write(struct pipe *pi) {
  return pi->nwrite != pi->nread + PIPESIZE;
}

int pipewrite(struct pipe *pi, uint64 addr, int n) {
  struct proc *p = myproc();
  int i = 0;

  acquire(&pi->lock);

  while (i < n) {

    if (pi->readopen == 0 || killed(p)) {
      release(&pi->lock);
      return -1;
    }

    if (!pipe_can_write(pi)) {
      wakeup(&pi->nread);
      sleep(&pi->nwrite, &pi->lock);
      continue;
    }

    char ch;
    if (copyin(p->pagetable, &ch, addr + i, 1) == -1)
      break;

    pi->data[pi->nwrite % PIPESIZE] = ch;
    pi->nwrite++;
    i++;
  }

  wakeup(&pi->nread);
  release(&pi->lock);

  return i;
}

/* ===================== READ ===================== */

static int pipe_empty(struct pipe *pi) {
  return pi->nread == pi->nwrite;
}

int piperead(struct pipe *pi, uint64 addr, int n) {
  struct proc *p = myproc();
  int i;
  char ch;

  acquire(&pi->lock);

  while (pipe_empty(pi) && pi->writeopen) {
    if (killed(p)) {
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock);
  }

  for (i = 0; i < n; i++) {

    if (pipe_empty(pi))
      break;

    ch = pi->data[pi->nread % PIPESIZE];

    if (copyout(p->pagetable, addr + i, &ch, 1) == -1) {
      if (i == 0)
        i = -1;
      break;
    }

    pi->nread++;
  }

  wakeup(&pi->nwrite);
  release(&pi->lock);

  return i;
}
