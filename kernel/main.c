#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    // fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }
  scheduler();        
}

// #include "../types.h"
// #include "../param.h"
// #include "../memlayout.h"
// #include "../riscv.h"
// #include "../spinlock.h"
// #include "../defs.h"
// #include "proc.h"

// struct cpu cpus[NCPU];

// struct proc_list {
//   struct proc_list* next;
//   struct proc_list* prev;
//   struct proc proc;
// };

// struct {
//   struct proc_list* procs;
//   struct proc_list* cemetery;
//   uint64 procs_size;
//   uint64 cemetery_size;
// } pdata;

// struct proc *initproc;

// int nextpid = 1;
// struct spinlock pid_lock;
// struct spinlock list_lock;
// struct spinlock hash_lock;
// struct spinlock free_lock;

// extern void forkret(void);
// static void freeproc(struct proc *p);

// extern char trampoline[]; // trampoline.S

// // helps ensure that wakeups of wait()ing
// // parents are not lost. helps obey the
// // memory model when using p->parent.
// // must be acquired before any p->lock.
// struct spinlock wait_lock;

// // Allocate a page for each process's kernel stack.
// // Map it high in memory, followed by an invalid
// // guard page.
// void
// proc_mapstacks(pagetable_t kpgtbl)
// {
  
// }

// // initialize the proc table.
// void
// procinit(void)
// {
//   initlock(&pid_lock, "nextpid");
//   initlock(&wait_lock, "wait_lock");
//   initlock(&list_lock, "list_lock");
//   initlock(&hash_lock, "hash_lock");
//   initlock(&free_lock, "free_lock");
//   pdata.procs = 0;
//   pdata.cemetery = 0;
//   pdata.procs_size = 0;
//   pdata.cemetery_size = 0;
//   proc_hashed_init();
// }

// // Must be called with interrupts disabled,
// // to prevent race with process being moved
// // to a different CPU.
// int
// cpuid()
// {
//   int id = r_tp();
//   return id;
// }

// // Return this CPU's cpu struct.
// // Interrupts must be disabled.
// struct cpu*
// mycpu(void)
// {
//   int id = cpuid();
//   struct cpu *c = &cpus[id];
//   return c;
// }

// // Return the current struct proc *, or zero if none.
// struct proc*
// myproc(void)
// {
//   push_off();
//   struct cpu *c = mycpu();
//   struct proc *p = c->proc;
//   pop_off();
//   return p;
// }

// int
// allocpid()
// {
//   int pid;
//   acquire(&pid_lock);
//   pid = nextpid;
//   nextpid = nextpid + 1;
//   release(&pid_lock);
//   return pid;
// }

// // list_lock must be held 
// // before entering this function
// int
// add_proc_to_cemetery(struct proc_list* proc) {
//   if (proc == 0) return -1;
//   proc->proc.state = ZOMBIE;
//   proc->next = pdata.cemetery;
//   if (pdata.cemetery != 0)
//     pdata.cemetery->prev = proc;
//   proc->prev = 0;
//   pdata.cemetery = proc;
//   pdata.cemetery_size++;
//   return 0;
// }

// // list_lock must be held.
// void dump_procs() {
// // printf("Procs:\n");
//   struct proc_list* current = pdata.procs;
//   while (current) {
//   // printf("  -> Proc %d %x", current->proc.pid, &current->proc);
//     if (current->next && current->next->prev == current) {
//     // printf(" ->");
//     }
//   // printf("\n");
//     current = current->next;
//   }
// // printf("Cemetery:\n");
//   current = pdata.cemetery;
//   while (current) {
//   // printf("  -> Proc %d %x", current->proc.pid, &current->proc);
//     if (current->next && current->next->prev == current) {
//     // printf(" ->");
//     }
//   // printf("\n");
//     current = current->next;
//   }
// }


// // Look in the process table for an UNUSED proc.
// // If found, initialize state required to run in the kernel,
// // and return with p->lock held.
// // If there are no free procs, or a memory allocation fails, return 0.
// static struct proc_list*
// allocproc()
// {

//   struct proc *p;
//   struct proc_list* pp;
//   // acquire(&free_lock);
//   // printf("as\n");
//   pp = bd_malloc(sizeof(struct proc_list));
//   // release(&free_lock);
//   // printf("ALLOCATED (%d, %d) SPACE AT %x \n", sizeof(*pp), sizeof(struct proc_list), pp);
//   if (pp == 0) panic("malloc");
//   int pid = allocpid();
//   // printf("bf %d %d \n", cpuid(), pid);

//   memset(pp, 0, sizeof(struct proc_list));
//   initlock(&pp->proc.lock, "proc");
//   acquire(&pp->proc.lock);

//   pp->prev = 0;
//   p = &pp->proc;
//   p->kstack = (uint64) (void*)bd_malloc(PGSIZE);
//   p->pid = pid;
//   p->state = USED;

//   // Allocate a trapframe page.
//   if((p->trapframe = (struct trapframe *)kalloc()) == 0) {
//     panic("1");
//     freeproc(p);
//     return 0;
//   }
//   // An empty user page table.
//   p->pagetable = proc_pagetable(p);
//   if(p->pagetable == 0){
//     panic("2");
//     freeproc(p);
//     return 0;
//   }

//   // Set up new context to start executing at forkret,
//   // which returns to user space.
//   memset(&p->context, 0, sizeof(p->context));
//   p->context.ra = (uint64)forkret;
//   p->context.sp = p->kstack + PGSIZE;
//   return pp;

// }

// // free a proc structure and the data hanging from it,
// // including user pages.
// // p->lock must be held.
// // list_lock must be held.
// static void
// freeproc(struct proc *p)
// {
//   // acquire(&free_lock);
//   // printf("p %d\n", p->pid);
//   struct proc_list* pp;
//   if (pdata.cemetery != 0) {
//     if (&pdata.cemetery->proc == p) {
//       pp = pdata.cemetery;
//       pdata.cemetery = pdata.cemetery->next;
//       if (pdata.cemetery != 0)
//         pdata.cemetery->prev = 0;
//     } else {
//       for (pp = pdata.cemetery->next; pp != 0; pp = pp->next) {
//         if (&pp->proc == p) {
//           pp->prev->next = pp->next;
//           if (pp->next != 0)
//             pp->next->prev = pp->prev;
//           break;
//         }
//       }
//     }
//     if(p->kstack) {
//       memset((void*)p->kstack, 0, PGSIZE);
//       bd_free((void*)p->kstack);
//     }
//     if(p->trapframe) {
//       kfree(p->trapframe);
//     }
//     if(p->pagetable && p->sz)
//       proc_freepagetable(p->pagetable, p->sz);
//     pdata.cemetery_size--;
//     acquire(&hash_lock);
//     struct proc_hashed* current_proc = proc_by_pid(p->pid);
//     current_proc->filled = 0;
//     current_proc->proc = 0;
//     release(&hash_lock);

//     // printf("fr %d %d \n", cpuid(), pp->proc.pid);
//     pop_off();
//     memset(pp, 0, sizeof(*pp));
//     bd_free((void*) pp );
//     // release(&free_lock);
//   } else {
//     panic("bad if it occurs");
//   }
//   return;
// }

// // Create a user page table for a given process, with no user memory,
// // but with trampoline and trapframe pages.
// pagetable_t
// proc_pagetable(struct proc *p)
// {
//   // printf("proc_pagetable\n");
//   pagetable_t pagetable;

//   // An empty page table.
//   pagetable = uvmcreate();
//   if(pagetable == 0)
//     return 0;

//   // map the trampoline code (for system call return)
//   // at the highest user virtual address.
//   // only the supervisor uses it, on the way
//   // to/from user space, so not PTE_U.
//   if(mappages(pagetable, TRAMPOLINE, PGSIZE,
//               (uint64)trampoline, PTE_R | PTE_X) < 0){
//     uvmfree(pagetable, 0);
//     return 0;
//   }

//   // map the trapframe page just below the trampoline page, for
//   // trampoline.S.
//   if(mappages(pagetable, TRAPFRAME, PGSIZE,
//               (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
//     uvmunmap(pagetable, TRAMPOLINE, 1, 0);
//     uvmfree(pagetable, 0);
//     return 0;
//   }

//   return pagetable;
// }

// // Free a process's page table, and free the
// // physical memory it refers to.
// void
// proc_freepagetable(pagetable_t pagetable, uint64 sz)
// {
//   uvmunmap(pagetable, TRAMPOLINE, 1, 0);
//   uvmunmap(pagetable, TRAPFRAME, 1, 0);
//   uvmfree(pagetable, sz);
// }

// // a user program that calls exec("/init")
// // assembled from ../user/initcode.S
// // od -t xC ../user/initcode
// uchar initcode[] = {
//   0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
//   0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
//   0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
//   0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
//   0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
//   0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
//   0x00, 0x00, 0x00, 0x00
// };

// // Set up first user process.
// void
// userinit(void)
// {
//   // printf("userinit\n");
//   struct proc_list *pp;
//   struct proc *p;
//   pp = allocproc();

//   acquire(&list_lock);
//   p = &pp->proc;
//   initproc = p;
//   pdata.procs = pp;
//   release(&list_lock);
  
//   // allocate one user page and copy initcode's instructions
//   // and data into it.
//   uvmfirst(p->pagetable, initcode, sizeof(initcode));
//   p->sz = PGSIZE;

//   // prepare for the very first "return" from kernel to user.
//   p->trapframe->epc = 0;      // user program counter
//   p->trapframe->sp = PGSIZE;  // user stack pointer

//   safestrcpy(p->name, "initcode", sizeof(p->name));
//   p->cwd = namei("/");

//   p->state = RUNNABLE;

//   acquire(&hash_lock);
//   create_proc_by_pid(p);
//   release(&hash_lock);
// // printf("userinit release \n");
//   release(&p->lock);

// }

// // Grow or shrink user memory by n bytes.
// // Return 0 on success, -1 on failure.
// int
// growproc(int n)
// {
//   // printf("growproc\n");
//   uint64 sz;
//   struct proc *p = myproc();

//   sz = p->sz;
//   if(n > 0){
//     if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
//       return -1;
//     }
//   } else if(n < 0){
//     sz = uvmdealloc(p->pagetable, sz, sz + n);
//   }
//   p->sz = sz;
//   return 0;
// }

// // Create a new process, copying the parent.
// // Sets up child kernel stack to return as if from fork() system call.
// int
// fork(void)
// {
//   // printf("fork start\n");
//   int i, pid;
//   struct proc* np;
//   struct proc_list* npp;
//   struct proc* p = myproc();

//   acquire(&list_lock);

//   // printf("%d %d \n", pdata.procs_size, pdata.cemetery_size );
//   if (pdata.procs_size + pdata.cemetery_size >= NPROC) {
//     release(&list_lock);
//     return -1;
//   } 

//   // printf("1");

//   // Allocate process.
//   if((npp = allocproc()) == 0) {
//     release(&list_lock);
//     return -1;
//   }

//   // printf("2");
//   pdata.procs->prev = npp;
//   npp->next = pdata.procs;
//   pdata.procs = npp;
//   pdata.procs_size++;
//   np = &npp->proc;

//   // printf("3");

//   // Copy user memory from parent to child.
//   if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
//     freeproc(np);
//     return -1;
//   }

//   release(&list_lock);

//   np->sz = p->sz;

//   // copy saved user registers.
//   *(np->trapframe) = *(p->trapframe);

//   // printf("4");
//   // Cause fork to return 0 in the child.
//   np->trapframe->a0 = 0;

//   // increment reference counts on open file descriptors.
//   for(i = 0; i < NOFILE; i++)
//     if(p->ofile[i])
//       np->ofile[i] = filedup(p->ofile[i]);
//   np->cwd = idup(p->cwd);

//   // printf("5");

//   safestrcpy(np->name, p->name, sizeof(p->name));

//   pid = np->pid;

//   // printf("6");
//   release(&np->lock);

//   acquire(&wait_lock);
//   np->parent = p;
//   release(&wait_lock);
//   // printf("7");
//   acquire(&np->lock);
//   np->state = RUNNABLE;

// // printf("procalloc2 release \n");
//   release(&np->lock);

//   acquire(&hash_lock);
//   // printf("fork end\n");
//   if(create_proc_by_pid(np) == PROC_WAS_FILLED) {
//     release(&hash_lock);
//     return pid;
//   }
//   release(&hash_lock);
//   return -1;

// }

// // Pass p's abandoned children to init.
// // Caller must hold wait_lock.
// void
// reparent(struct proc *p)
// {
//   struct proc_list *pp;
//   acquire(&list_lock);
//   for(pp = pdata.procs; pp != 0; pp = pp->next){
//     if(pp->proc.parent == p){
//       pp->proc.parent = initproc;
//       wakeup(initproc);
//     }
//   }

//   // если родительский процесс умер
//   // убираем его детей с кладбища...
//   for(pp = pdata.cemetery; pp != 0; pp = pp->next){
//     if(pp->proc.parent == p){
//       pp->proc.parent = initproc;
//       wakeup(initproc);
//     }
//   }
//   release(&list_lock);
// }


// // Exit the current process.  Does not return.
// // An exited process remains in the zombie state
// // until its parent calls wait().
// void
// exit(int status)
// {
//   struct proc *p = myproc();
//   // printf("exit %d proc\n", p->pid);

//   if(p == initproc)
//     panic("init exiting");

//   // Close all open files.
//   for(int fd = 0; fd < NOFILE; fd++){
//     if(p->ofile[fd]){
//       struct file *f = p->ofile[fd];
//       fileclose(f);
//       p->ofile[fd] = 0;
//     }
//   }

//   begin_op();
//   iput(p->cwd);
//   end_op();
//   p->cwd = 0;

//   acquire(&wait_lock);

//   // Give any children to init.
//   reparent(p);

//   // Parent might be sleeping in wait().
//   wakeup(p->parent);
  
//   acquire(&p->lock);

//   p->xstate = status;
//   p->state = ZOMBIE;

// // printf("exit release \n");
//     release(&p->lock);

//   acquire(&list_lock);
//   pdata.procs_size--;
//   struct proc_list* pp;
//   if (&pdata.procs->proc == p) {
//     pp = pdata.procs;
//     pdata.procs = pdata.procs->next;
//     if (pdata.procs != 0)
//         pdata.procs->prev = 0;
//   } else {
//     for (pp = pdata.procs->next; pp != 0; pp = pp->next) {
//       if (&pp->proc == p) {
//         pp->prev->next = pp->next;
//         if (pp->next != 0)
//           pp->next->prev = pp->prev;
//         break;
//       }
//     }
//   }
//   add_proc_to_cemetery(pp);
//   release(&list_lock);
  
//   release(&wait_lock);
  
//   acquire(&p->lock);
//   // Jump into the scheduler, never to return.

//   sched();
  
//   panic("zombie exit");
// }

// // Wait for a child process to exit and return its pid.
// // Return -1 if this process has no children.
// int
// wait(uint64 addr)
// {

//   struct proc *pp;
//   struct proc_list *ppl;
//   int havekids, pid;
//   struct proc *p = myproc();

//   // printf("STARTED wait_lock, wait CPU: %d\n", cpuid());
//   acquire(&wait_lock);

//   for(;;){
//     // Scan through table looking for exited children.
//     havekids = 0; 

//     acquire(&list_lock);
//     for(ppl = pdata.procs; ppl != 0; ppl = ppl->next) {
//       pp = &ppl->proc;
//       acquire(&pp->lock);
//       if(pp->parent == p)
//         havekids = 1;
//     // printf("wait release \n");
//         release(&pp->lock);
//     }
//     for(ppl = pdata.cemetery; ppl != 0; ppl = ppl->next) {
//       pp = &ppl->proc;
//       acquire(&pp->lock);
//       if(pp->parent == p){
//         havekids = 1;
//         if(pp->state == ZOMBIE){
//           // Found one.
//           pid = pp->pid;
//           if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
//                                   sizeof(pp->xstate)) < 0) {
//           // printf("wait1 release \n");
//               release(&pp->lock);
//             release(&wait_lock);
//             release(&list_lock);
//             return -1;
//           }
//           release(&wait_lock);
//           release(&list_lock);
//           freeproc(pp); 
//           return pid;
//         }
//       // printf("wait2 release \n");
//           release(&pp->lock); 
//       }
//     }
//     release(&list_lock);

//     // No point waiting if we don't have any children.
//     if(!havekids || killed(p)){
//       release(&wait_lock);
//       return -1;
//     }
  
//     // Wait for a child to exit.
//     sleep(p, &wait_lock);  //DOC: wait-sleep

//   }
// }

// // Per-CPU process scheduler.
// // Each CPU calls scheduler() after setting itself up.
// // Scheduler never returns.  It loops, doing:
// //  - choose a process to run.
// //  - swtch to start running that process.
// //  - eventually that process transfers control
// //    via swtch back to the scheduler.
// void
// scheduler(void)
// {
//   // printf("scheduler %d\n", cpuid());
//   struct proc *p;
//   struct proc_list* pp;
//   struct cpu *c = mycpu();
  
//   c->proc = 0;
//   for(;;){
//     // printf("schedule loop\n");
//     // Avoid deadlock by ensuring that devices can interrupt.
//     intr_on();
//     // acquire(&list_lock);
//     for(pp = pdata.procs; pp != 0; pp = pp->next) {
//       p = &pp->proc;
//       // printf("STARTED p->lock, scheduler CPU: %d\n", cpuid());
//       acquire(&p->lock);
//       if(p->state == RUNNABLE) {
//         // Switch to chosen process.  It is the process's job
//         // to release its lock and then reacquire it
//         // before jumping back to us.
//         p->state = RUNNING;
//         c->proc = p;
//         // printf("CONTEXT: %x\n", p->context);
//         swtch(&c->context, &p->context);

//         // Process is done running for now.
//         // It should have changed its p->state before coming back.
//         c->proc = 0;
//       }
//       // printf("scheduler release \n");
//         release(&p->lock);
//       // printf("FINISHED p->lock, scheduler CPU: %d\n", cpuid());
//     }
//     // release(&list_lock);
//   }
// }

// // Switch to scheduler.  Must hold only p->lock
// // and have changed proc->state. Saves and restores
// // intena because intena is a property of this
// // kernel thread, not this CPU. It should
// // be proc->intena and proc->noff, but that would
// // break in the few places where a lock is held but
// // there's no process.
// void
// sched(void)
// {

//   // printf("sched\n");

//   int intena;
//   struct proc *p = myproc();

//   if(!holding(&p->lock))
//     panic("sched p->lock");
//   if(mycpu()->noff != 1)
//     panic("sched locks");
//   if(p->state == RUNNING)
//     panic("sched running");
//   if(intr_get())
//     panic("sched interruptible");

//   intena = mycpu()->intena;
//   swtch(&p->context, &mycpu()->context);
//   mycpu()->intena = intena;
// }

// // Give up the CPU for one scheduling round.
// void
// yield(void)
// {
//   // printf("yield\n");
//   struct proc *p = myproc();
//   acquire(&p->lock);
//   p->state = RUNNABLE;
//   sched();
// // printf("yield release \n");
//     release(&p->lock);
// }

// // A fork child's very first scheduling by scheduler()
// // will swtch to forkret.
// void
// forkret(void)
// {
//   static int first = 1;

//   // Still holding p->lock from scheduler.
// // printf("forkret release \n");
//     release(&myproc()->lock);

//   if (first) {
//     // File system initialization must be run in the context of a
//     // regular process (e.g., because it calls sleep), and thus cannot
//     // be run from main().
//     first = 0;
//     fsinit(ROOTDEV);
//   }
//   usertrapret();
// }

// // Atomically release lock and sleep on chan.
// // Reacquires lock when awakened.
// void
// sleep(void *chan, struct spinlock *lk)
// {
//   // printf("sleep\n");
//   struct proc *p = myproc();
  
//   // Must acquire p->lock in order to
//   // change p->state and then call sched.
//   // Once we hold p->lock, we can be
//   // guaranteed that we won't miss any wakeup
//   // (wakeup locks p->lock),
//   // so it's okay to release lk.

//   acquire(&p->lock);  //DOC: sleeplock1
//   // printf("sleep release \n");
//     release(lk);

//   // Go to sleep.
//   p->chan = chan;
//   p->state = SLEEPING;

//   sched();

//   // Tidy up.
//   p->chan = 0;

//   // Reacquire original lock.
//   // printf("sleep1 release \n");
//     release(&p->lock);
//   acquire(lk);
// }

// // Wake up all processes sleeping on chan.
// // Must be called without any p->lock.
// void
// wakeup(void *chan)
// {
//   // printf("wakeup %x %x\n", cpuid(), chan);
//   struct proc *p;
//   struct proc_list* pp;
//   for(pp = pdata.procs; pp != 0; pp = pp->next) {
//     p = &pp->proc;
//     if (p != myproc()) {
//       // printf("STARTED p->lock, wakeup CPU: %d\n", cpuid());
//       acquire(&p->lock);
//       if(p->state == SLEEPING && p->chan == chan) {
//         p->state = RUNNABLE;
//       }
//       // printf("wakeup release \n");
//         release(&p->lock);
//     }
//   }

// }

// // Kill the process with the given pid.
// // The victim won't exit until it tries to return
// // to user space (see usertrap() in trap.c).
// int
// kill(int pid)
// {
//   // printf("kill\n");
//   acquire(&hash_lock);
//   struct proc_hashed *pp = proc_by_pid(pid);
//   release(&hash_lock);
//   if (pp != 0 && pp->proc != 0) {
//     struct proc* p = pp->proc;
//     acquire(&p->lock);
//     p->killed = 1;
//     if(p->state == SLEEPING){
//       // Wake process from sleep().
//       p->state = RUNNABLE;
//     }
//   // printf("kill release \n");
//       release(&p->lock);
//     return 0;
//   }
//   return -1;
// }

// void
// setkilled(struct proc *p)
// {
//   // printf("setkilled\n");
//   acquire(&p->lock);
//   p->killed = 1;
// // printf("setkilled release \n");
//     release(&p->lock);
// }

// int
// killed(struct proc *p)
// {
//   // printf("killed\n");
//   int k;
  
//   acquire(&p->lock);
//   k = p->killed;
//     release(&p->lock);
//   return k;
// }

// // Copy to either a user address, or kernel address,
// // depending on usr_dst.
// // Returns 0 on success, -1 on error.
// int
// either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
// {
//   // printf("either_copyout\n");
//   struct proc *p = myproc();
//   if(user_dst){
//     return copyout(p->pagetable, dst, src, len);
//   } else {
//     memmove((char *)dst, src, len);
//     return 0;
//   }
// }

// // Copy from either a user address, or kernel address,
// // depending on usr_src.
// // Returns 0 on success, -1 on error.
// int
// either_copyin(void *dst, int user_src, uint64 src, uint64 len)
// {
//   // printf("either_copyin\n");
//   struct proc *p = myproc();
//   if(user_src){
//     return copyin(p->pagetable, dst, src, len);
//   } else {
//     memmove(dst, (char*)src, len);
//     return 0;
//   }
// }

// // Print a process listing to console.  For debugging.
// // Runs when user types ^P on console.
// // No lock to avoid wedging a stuck machine further.
// void
// procdump(void)
// {
//   // printf("START PROC DUMP\n");
//   static char *states[] = {
//   [UNUSED]    "unused",
//   [USED]      "used",
//   [SLEEPING]  "sleep ",
//   [RUNNABLE]  "runble",
//   [RUNNING]   "run   ",
//   [ZOMBIE]    "zombie"
//   };
//   struct proc *p;
//   struct proc_list* pp;
//   char *state;

//   for(pp = pdata.procs; pp != 0; pp = pp->next) {
//     p = &pp->proc;
//     if(p->state == UNUSED)
//       continue;
//     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
//       state = states[p->state];
//     else
//       state = "???";
//     printf("%d %s %s \n", p->pid, state, p->name);
//   }

// // printf("CEMETERY: \n");
//   for(pp = pdata.cemetery; pp != 0; pp = pp->next) {
//     p = &pp->proc;
//     if(p->state == UNUSED)
//       continue;
//     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
//       state = states[p->state];
//     else
//       state = "???";
//   // printf("%d %s %s \n", p->pid, state, p->name);
//   }
//   // printf("END DUMP\n");
// }

// // Prints states of registers s2 - s11 to console if hex format
// // Returns 0 if successfully printed
// int dump(void) {
//   printf("freed");
//   free_hash_table();
//   dump_procs();
//   return 0; 
// // printf("dump\n");
//   struct proc* current_proc = myproc();
//   struct trapframe* current_trapframe = current_proc->trapframe;
//   if (current_trapframe != 0) {
//     int current_register = 2;
//     for (uint64* i = &(current_trapframe->s2); i <= &(current_trapframe->s11); i++) {
//     // printf("s%d = %d\n", current_register, *((int*) i));
//       current_register++;
//     }
//     return 0;
//   }
//   return -1;

// }

// // Gets value in register with register_num of target proccess
// // with given pid. Result is written in return_value address
// // Returns 0 if successfully written
// // Returns -1 if proccess cannot be accessed
// // Returns -2 if there is no proccess with pid
// // Returns -3 if there is no such register from s2 - s11
// // Returns -4 if return_value is incorrect for writing
// int dump2(int pid, int register_num, uint64 return_value) {

// // printf("dump2\n");

//   // printf("%d %d %d \n", pid, register_num, return_value);

//   // If passed incorrect data
//   if (register_num < 2 || register_num > 11) return DUMP2_INCORRECT_REGISTER_NUMBER;

//   // Try to find proccess with given PID on CPU
//   struct proc_hashed* proc_hashed = proc_by_pid(pid);
//   if (proc_hashed == PROC_NO_PROCESS_FOUND) return DUMP2_INCORRECT_PID;
//   struct proc* target_proc = proc_hashed->proc;
//   if (target_proc == PROC_NO_PROCESS_FOUND) return DUMP2_INCORRECT_PID; 
//   struct proc * current_proc = myproc();

//   // Checking whether target process is child process
//   struct proc * t_proc = target_proc;
//   while (t_proc != 0 && t_proc->pid != current_proc->pid) 
//     t_proc = t_proc->parent;
//   if (t_proc == 0) return DUMP2_PROCCESS_CANNOT_BE_ACCESSED;

//   // Writing register value to &target_value
//   struct trapframe* target_trapframe = target_proc->trapframe;
//   uint64 value = *(&(target_trapframe->s2) + (register_num - 2));
//   int copy_result = copyout(current_proc->pagetable, return_value, (char*) &value, sizeof(uint64));
//   if (copy_result == -1) return DUMP2_UNABLE_TO_WRITE;

//   return 0;
// }

