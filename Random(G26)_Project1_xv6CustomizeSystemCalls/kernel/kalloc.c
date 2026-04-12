#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[]; // first address after kernel

/* ===================== DATA STRUCTURES ===================== */

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

/* ===================== INTERNAL HELPERS ===================== */

// Check if a physical address is valid for freeing
static int is_valid_pa(void *pa) {
  return ((uint64)pa % PGSIZE == 0) &&
         ((char *)pa >= end) &&
         ((uint64)pa < PHYSTOP);
}

// Push a page into freelist (caller must hold lock)
static void freelist_push(struct run *r) {
  r->next = kmem.freelist;
  kmem.freelist = r;
}

// Pop a page from freelist (caller must hold lock)
static struct run* freelist_pop() {
  struct run *r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  return r;
}

/* ===================== INITIALIZATION ===================== */

void freerange(void *pa_start, void *pa_end) {
  char *p = (char *)PGROUNDUP((uint64)pa_start);

  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

void kinit() {
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

/* ===================== FREE ===================== */

void kfree(void *pa) {
  if (!is_valid_pa(pa))
    panic("kfree");

  // Fill with junk to catch dangling references
  memset(pa, 1, PGSIZE);

  struct run *r = (struct run *)pa;

  acquire(&kmem.lock);
  freelist_push(r);
  release(&kmem.lock);
}

/* ===================== ALLOC ===================== */

void *kalloc(void) {
  acquire(&kmem.lock);
  struct run *r = freelist_pop();
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk

  return (void *)r;
}
