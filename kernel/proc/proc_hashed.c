#include "../types.h"
#include "../param.h"
#include "../memlayout.h"
#include "../riscv.h"
#include "../spinlock.h"
#include "../defs.h"
#include "proc.h"

struct {
  struct proc_hashed* procs;
  int array_size;
} phtable;

void
proc_hashed_init(void) {

  phtable.array_size = NPROC;
  int table_size = phtable.array_size * sizeof(struct proc_hashed);
  phtable.procs = bd_malloc(table_size);
  memset(phtable.procs, 0, table_size);

}

int
proc_expand_array_size(void) {

  int new_procs_size = EXPANDRATE * phtable.array_size * sizeof(struct proc_hashed);
  struct proc_hashed* old_procs = phtable.procs;
  int old_array_size = phtable.array_size;
  phtable.procs = bd_malloc(new_procs_size);
  memset(phtable.procs, 0, new_procs_size);
  if (phtable.procs == 0) {
    return ARRAY_CANNOT_BE_EXTENDED;
  }
  phtable.array_size *= EXPANDRATE;
  
  struct proc_hashed* p;
  for (p = old_procs; p < &old_procs[old_array_size]; p++) {
    if (p->proc == 0 || !p->filled) continue;
    create_proc_by_pid(p->proc);
  }

  memset(old_procs, 0, old_array_size * sizeof(struct proc_hashed));
  bd_free(old_procs);
  
  return PROC_WAS_FILLED;
  
}

int 
proc_hash(int pid) {
  return ((17 * pid + 31) % phtable.array_size + phtable.array_size) % phtable.array_size;
}

struct proc_hashed* 
proc_by_pid(int pid) {
  int hash = proc_hash(pid);
  for (int i = 0; i < phtable.array_size; i++) {
    struct proc_hashed selected_proc = phtable.procs[(hash + i) % phtable.array_size];
    if (!selected_proc.filled || selected_proc.proc->pid == pid) {
      return &phtable.procs[(hash + i) % phtable.array_size];
    }
  }
  return PROC_NO_PROCESS_FOUND;
} 

int 
create_proc_by_pid(struct proc *current_process) {
  int hash = proc_hash(current_process->pid);
  for (int i = 0; i < phtable.array_size; i++) {
    struct proc_hashed selected_proc = phtable.procs[(hash + i) % phtable.array_size];
    if (!selected_proc.filled) {
      phtable.procs[(hash + i) % phtable.array_size] = (struct proc_hashed) {
        current_process->pid,
        1,
        current_process
      };
      return PROC_WAS_FILLED;
    }
  }
  if (proc_expand_array_size() == 0) {
    return create_proc_by_pid(current_process);
  }
  return PROC_NO_SPACE_FOR_PROCESS;
}

int 
print_hash_proc(void) {
  struct proc_hashed* p; int i = 0;
  for (p = phtable.procs; p < &phtable.procs[phtable.array_size]; p++) {
    printf("%d -> (FILLED: %d, PID, %d ADDR: %x)\n", i++, p->filled, p->pid, p);
  }
  return PROC_OK;
}

int
free_hash_table(void) {
  memset(phtable.procs, 0, phtable.array_size * sizeof(struct proc_hashed));
  bd_free(phtable.procs);
  return 0;
}