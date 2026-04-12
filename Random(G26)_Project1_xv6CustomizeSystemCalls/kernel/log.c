#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

/* ===================== LOG STRUCTURES ===================== */

struct logheader {
  int n;
  int block[LOGBLOCKS];
};

struct log {
  struct spinlock lock;
  int start;
  int outstanding;
  int committing;
  int dev;
  struct logheader lh;
};

struct log log;

/* ===================== FORWARD DECL ===================== */

static void commit(void);
static void install_trans(int recovering);
static void read_head(void);
static void write_head(void);

/* ===================== INIT ===================== */

void initlog(int dev, struct superblock *sb) {
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log.lock, "log");
  log.start = sb->logstart;
  log.dev = dev;

  install_trans(1);
}

/* ===================== LOG HEADER I/O ===================== */

static void read_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *)buf->data;

  log.lh.n = lh->n;
  for (int i = 0; i < log.lh.n; i++)
    log.lh.block[i] = lh->block[i];

  brelse(buf);
}

static void write_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *)buf->data;

  hb->n = log.lh.n;
  for (int i = 0; i < log.lh.n; i++)
    hb->block[i] = log.lh.block[i];

  bwrite(buf);
  brelse(buf);
}

/* ===================== INSTALL ===================== */

static void install_trans(int recovering) {
  for (int i = 0; i < log.lh.n; i++) {

    if (recovering)
      printf("recovering block %d -> %d\n", i, log.lh.block[i]);

    struct buf *lbuf = bread(log.dev, log.start + i + 1);
    struct buf *dbuf = bread(log.dev, log.lh.block[i]);

    memmove(dbuf->data, lbuf->data, BSIZE);
    bwrite(dbuf);

    if (!recovering)
      bunpin(dbuf);

    brelse(lbuf);
    brelse(dbuf);
  }
}

/* ===================== RECOVERY ===================== */

static void recover_from_log(void) {
  read_head();
  install_trans(1);
  log.lh.n = 0;
  write_head();
}

/* ===================== TRANSACTION CONTROL ===================== */

void begin_op(void) {
  acquire(&log.lock);

  while (1) {
    if (log.committing) {
      sleep(&log, &log.lock);
    } 
    else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGBLOCKS) {
      sleep(&log, &log.lock);
    } 
    else {
      log.outstanding++;
      release(&log.lock);
      break;
    }
  }
}

void end_op(void) {
  int do_commit = 0;

  acquire(&log.lock);

  log.outstanding--;

  if (log.committing)
    panic("log.committing");

  if (log.outstanding == 0) {
    do_commit = 1;
    log.committing = 1;
  } 
  else {
    wakeup(&log);
  }

  release(&log.lock);

  if (do_commit) {
    commit();

    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

/* ===================== WRITE LOG ===================== */

static void write_log(void) {
  for (int i = 0; i < log.lh.n; i++) {
    struct buf *to = bread(log.dev, log.start + i + 1);
    struct buf *from = bread(log.dev, log.lh.block[i]);

    memmove(to->data, from->data, BSIZE);
    bwrite(to);

    brelse(from);
    brelse(to);
  }
}

/* ===================== COMMIT ===================== */

static void commit(void) {
  if (log.lh.n == 0)
    return;

  write_log();      // 1. write to log
  write_head();     // 2. commit header
  install_trans(0); // 3. install to FS

  log.lh.n = 0;
  write_head();     // 4. clear log
}

/* ===================== LOG WRITE ===================== */

void log_write(struct buf *b) {
  acquire(&log.lock);

  if (log.lh.n >= LOGBLOCKS)
    panic("too big transaction");

  if (log.outstanding < 1)
    panic("log_write outside transaction");

  int i;

  // log absorption
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)
      break;
  }

  log.lh.block[i] = b->blockno;

  if (i == log.lh.n) {
    bpin(b);
    log.lh.n++;
  }

  release(&log.lock);
}
