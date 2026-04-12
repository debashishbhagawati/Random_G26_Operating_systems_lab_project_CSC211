#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];
void kernelvec();
extern int devintr();

/* ===================== INIT ===================== */

void trapinit(void) {
  initlock(&tickslock, "time");
}

void trapinithart(void) {
  w_stvec((uint64)kernelvec);
}

/* ===================== DEVICE HELPERS ===================== */

static void handle_syscall(struct proc *p) {
  if (killed(p))
    kexit(-1);

  p->trapframe->epc += 4;
  intr_on();
  syscall();
}

static void handle_page_fault(struct proc *p, uint64 scause) {
  int write = (scause == 15);

  if (!vmfault(p->pagetable, r_stval(), write)) {
    setkilled(p);
  }
}

/* ===================== USER TRAP ===================== */

uint64 usertrap(void) {
  if (r_sstatus() & SSTATUS_SPP)
    panic("usertrap: not from user mode");

  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  p->trapframe->epc = r_sepc();

  int which_dev = 0;

  uint64 scause = r_scause();

  if (scause == 8) {
    handle_syscall(p);

  } else if ((which_dev = devintr()) != 0) {
    // device interrupt handled

  } else if ((scause == 15 || scause == 13)) {
    handle_page_fault(p, scause);

  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n",
           scause, p->pid);
    printf("sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if (killed(p))
    kexit(-1);

  if (which_dev == 2)
    yield();

  prepare_return();

  return MAKE_SATP(p->pagetable);
}

/* ===================== RETURN TO USER ===================== */

void prepare_return(void) {
  struct proc *p = myproc();

  intr_off();

  w_stvec(TRAMPOLINE + (uservec - trampoline));

  p->trapframe->kernel_satp = r_satp();
  p->trapframe->kernel_sp = p->kstack + PGSIZE;
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();

  uint64 x = r_sstatus();
  x &= ~SSTATUS_SPP;
  x |= SSTATUS_SPIE;
  w_sstatus(x);

  w_sepc(p->trapframe->epc);
}

/* ===================== KERNEL TRAP ===================== */

void kerneltrap(void) {
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");

  if (intr_get())
    panic("kerneltrap: interrupts enabled");

  int which_dev = devintr();

  if (which_dev == 0) {
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n",
           scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  if (which_dev == 2 && myproc())
    yield();

  w_sepc(sepc);
  w_sstatus(sstatus);
}

/* ===================== TIMER INTERRUPT ===================== */

void clockintr(void) {
  if (cpuid() == 0) {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  w_stimecmp(r_time() + 1000000);
}

/* ===================== DEVICE INTERRUPT ===================== */

int devintr(void) {
  uint64 scause = r_scause();

  /* External interrupt */
  if (scause == 0x8000000000000009L) {
    int irq = plic_claim();

    if (irq == UART0_IRQ)
      uartintr();
    else if (irq == VIRTIO0_IRQ)
      virtio_disk_intr();
    else if (irq)
      printf("unexpected interrupt irq=%d\n", irq);

    if (irq)
      plic_complete(irq);

    return 1;
  }

  /* Timer interrupt */
  if (scause == 0x8000000000000005L) {
    clockintr();
    return 2;
  }

  return 0;
}
