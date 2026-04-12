#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

struct devsw devsw[NDEV];

struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

/* ===================== INIT ===================== */

void fileinit(void) {
  initlock(&ftable.lock, "ftable");
}

/* ===================== FILE TABLE HELPERS ===================== */

static struct file* file_find_free() {
  for (struct file *f = ftable.file;
       f < ftable.file + NFILE;
       f++) {
    if (f->ref == 0) {
      f->ref = 1;
      return f;
    }
  }
  return 0;
}

/* ===================== FILE OPERATIONS ===================== */

struct file* filealloc(void) {
  struct file *f;

  acquire(&ftable.lock);
  f = file_find_free();
  release(&ftable.lock);

  return f;
}

struct file* filedup(struct file *f) {
  acquire(&ftable.lock);

  if (f->ref < 1)
    panic("filedup");

  f->ref++;

  release(&ftable.lock);
  return f;
}

void fileclose(struct file *f) {
  struct file ff;

  acquire(&ftable.lock);

  if (f->ref < 1)
    panic("fileclose");

  if (--f->ref > 0) {
    release(&ftable.lock);
    return;
  }

  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;

  release(&ftable.lock);

  if (ff.type == FD_PIPE) {
    pipeclose(ff.pipe, ff.writable);
  } 
  else if (ff.type == FD_INODE || ff.type == FD_DEVICE) {
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

/* ===================== FILE METADATA ===================== */

int filestat(struct file *f, uint64 addr) {
  struct proc *p = myproc();
  struct stat st;

  if (f->type != FD_INODE && f->type != FD_DEVICE)
    return -1;

  ilock(f->ip);
  stati(f->ip, &st);
  iunlock(f->ip);

  return (copyout(p->pagetable, addr,
                  (char *)&st, sizeof(st)) < 0) ? -1 : 0;
}

/* ===================== READ ===================== */

static int fileread_pipe(struct file *f, uint64 addr, int n) {
  return piperead(f->pipe, addr, n);
}

static int fileread_device(struct file *f, uint64 addr, int n) {
  if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
    return -1;
  return devsw[f->major].read(1, addr, n);
}

static int fileread_inode(struct file *f, uint64 addr, int n) {
  int r;

  ilock(f->ip);
  r = readi(f->ip, 1, addr, f->off, n);
  if (r > 0)
    f->off += r;
  iunlock(f->ip);

  return r;
}

int fileread(struct file *f, uint64 addr, int n) {
  if (!f->readable)
    return -1;

  switch (f->type) {
    case FD_PIPE:   return fileread_pipe(f, addr, n);
    case FD_DEVICE: return fileread_device(f, addr, n);
    case FD_INODE:  return fileread_inode(f, addr, n);
    default:        panic("fileread");
  }

  return -1;
}

/* ===================== WRITE ===================== */

static int filewrite_device(struct file *f, uint64 addr, int n) {
  if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
    return -1;
  return devsw[f->major].write(1, addr, n);
}

static int filewrite_pipe(struct file *f, uint64 addr, int n) {
  return pipewrite(f->pipe, addr, n);
}

static int filewrite_inode(struct file *f, uint64 addr, int n) {
  int i = 0, r;

  int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;

  while (i < n) {
    int n1 = n - i;
    if (n1 > max)
      n1 = max;

    begin_op();
    ilock(f->ip);

    r = writei(f->ip, 1, addr + i, f->off, n1);
    if (r > 0)
      f->off += r;

    iunlock(f->ip);
    end_op();

    if (r != n1)
      break;

    i += r;
  }

  return (i == n) ? n : -1;
}

int filewrite(struct file *f, uint64 addr, int n) {
  if (!f->writable)
    return -1;

  switch (f->type) {
    case FD_PIPE:   return filewrite_pipe(f, addr, n);
    case FD_DEVICE: return filewrite_device(f, addr, n);
    case FD_INODE:  return filewrite_inode(f, addr, n);
    default:        panic("filewrite");
  }

  return -1;
}
