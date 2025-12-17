#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;
  // pte_t *pte;
  // uint64 a, last;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  // printf("before: %p sz: %p\r\n", addr, myproc()->sz);

  // myproc()->sz = addr + n;
  // if(myproc()->sz > MAXVA){
  //   // printf("oom ");
  //   myproc()->sz = MAXVA;
  //   return -1;
  // }

  if(n >= 0 && addr + n >= addr){
    // printf("growproc: %p\r\n", addr + n);
    myproc()->sz += n;    // increase size but not allocate memory
  } else if(n < 0 && addr + n >= PGROUNDUP(myproc()->trapframe->sp)){
    // handle negative n and addr must be above user stack - lab5-3
    myproc()->sz = uvmdealloc(myproc()->pagetable, addr, addr + n);
  } else {
    return -1;
  }
  
  // if(n < 0){
  //   // printf("sys_brk arg negative: %d | pid: %d | ptbl: %p\r\n", n, myproc()->pid, myproc()->pagetable);
  //   uvmunmap(myproc()->pagetable, myproc()->sz + PGSIZE, (-n)/PGSIZE, 1);
  //   // vmprint(myproc()->pagetable);
  //   // printf("ok\r\n");
  // }
  
  // if(growproc(n) < 0)
  //   return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
