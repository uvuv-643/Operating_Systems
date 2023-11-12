#include "../types.h"
#include "../param.h"
#include "../memlayout.h"
#include "../riscv.h"
#include "../spinlock.h"
#include "../defs.h"
#include "proc.h"

struct proc_pool {
    struct proc_list proc_lists[NPROC];
    struct proc_pool* prev;
    struct proc_pool* next;
};

// proc_index indicates where was put element last time
struct proc_pool* last_pool = 0;
int proc_index = 0;

// how often after free system has to find unused pools
int proc_interval = NPROC / 2;
int proc_interval_current = 0;
int min_active_proc_count_in_pool = NPROC / 2;

struct proc_pool* head;
struct spinlock pool_lock;
struct spinlock interval_lock;
struct spinlock pool_counters_lock;


// init base memory pool for all the processed
// function must be called once when the proccess module starts
// no lock has to be acquired
void proc_pool_init() {
    initlock(&pool_lock, "pool_lock");
    initlock(&interval_lock, "interval_lock");
    initlock(&pool_counters_lock, "pool_counters_lock");
    head = (struct proc_pool* ) bd_malloc(sizeof(struct proc_pool));
    last_pool = head;
    proc_index = -1;
    memset(head, 0, sizeof(struct proc_pool));
}


// if there are many pools in system, the function frees unused pools
// pool is unused when all the processes have state "UNUSED"
// no lock has to be acquired before entering function
void free_unused_proc_pools() {

    proc_pool_dump();

    acquire(&pool_lock);

    // that is pools which will be merged (?) by this function
    struct proc_pool* first_small_pool = 0;
    int first_small_pool_cnt = 0;
    struct proc_pool* second_small_pool = 0;
    int second_small_pool_cnt = 0;

    struct proc_pool* current_pool = head->next;

    // there we find two smallest pools for merging
    // if merge is possible
    // first_small_pool != 0 && second_small_pool != 0
    while (current_pool != 0) {
        int number_of_used_procs = 0;
        for (int i = 0; i < NPROC; i++) {
            if (current_pool->proc_lists->proc.state != UNUSED) {
                number_of_used_procs++;
                break;
            }
        }
        if (number_of_used_procs < min_active_proc_count_in_pool) {
            if (first_small_pool && !second_small_pool) {
                second_small_pool = current_pool;
                second_small_pool_cnt = number_of_used_procs;
            }
            else if (!first_small_pool) {
                first_small_pool = current_pool;
                first_small_pool_cnt = number_of_used_procs;
            } else {
                if (first_small_pool_cnt > second_small_pool_cnt) {
                    if (number_of_used_procs < first_small_pool_cnt) {
                        first_small_pool = current_pool;
                        first_small_pool_cnt = number_of_used_procs;
                    }
                } else {
                    if (number_of_used_procs < second_small_pool_cnt) {
                        second_small_pool = current_pool;
                        second_small_pool_cnt = number_of_used_procs;
                    }
                }
            }
        }
        current_pool = current_pool->next;
    }

    // merge two pools
    if (first_small_pool != 0 && second_small_pool != 0 && first_small_pool_cnt + second_small_pool_cnt <= NPROC) {
        struct proc_pool* pool = (struct proc_pool*) bd_malloc(sizeof(struct proc_pool));
        memset(head, 0, sizeof(struct proc_pool));
        int l = 0; int r = 0;
        while (l < first_small_pool_cnt && r < second_small_pool_cnt) {
            if (l < first_small_pool_cnt && r < second_small_pool_cnt) {
                if (first_small_pool->proc_lists[l].proc.pid < second_small_pool->proc_lists[r].proc.pid) {
                    pool->proc_lists[l + r] = first_small_pool->proc_lists[l];
                    l++;
                } else {
                    pool->proc_lists[l + r] = second_small_pool->proc_lists[r];
                    r++;
                }
            }
        }
        
        // after merging pools to a new pool we have to remove old
        // pools from list
        pool->next = first_small_pool->next;
        pool->prev = first_small_pool->prev;
        first_small_pool->prev->next = pool;

        struct proc_pool* pp = second_small_pool;
        second_small_pool->prev->next = second_small_pool->next;
        second_small_pool->next->prev = second_small_pool->prev;

        bd_free(pp);
        bd_free(first_small_pool);

    }

    proc_interval_current = 0;
    release(&pool_lock);
}


struct proc_list* take_from_pool() { 
    struct proc_pool* pp = last_pool;
    acquire(&pool_counters_lock);
    proc_index += 1;
    release(&pool_counters_lock);
    if ((proc_index) >= NPROC) {
        // alloc more memory because of pool overflow
        struct proc_pool* np = (struct proc_pool*) bd_malloc(sizeof(struct proc_pool));
        pp->next = np;
        np->prev = pp;
        acquire(&pool_counters_lock);
        last_pool = np;
        proc_index = 0;
        release(&pool_counters_lock);
    } else {
        
    }
    return &last_pool->proc_lists[proc_index];
}

void free_from_pool(struct proc_list* proc) {
    acquire(&proc->proc.lock);
    proc->proc.state = UNUSED;
    release(&proc->proc.lock);
    
    acquire(&interval_lock);
    proc_interval_current++;
    if (proc_interval_current >= proc_interval) {
        free_unused_proc_pools();
        proc_interval_current = 0;
    }
    release(&interval_lock);

}

void proc_pool_dump() {
    static char* states[] = {
        [UNUSED]    "0",
        [USED]      "U",
        [SLEEPING]  "S",
        [RUNNABLE]  "W",
        [RUNNING]   "R",
        [ZOMBIE]    "Z"
    };
    struct proc_pool* pp = head;
    printf("last_pool %p, last_index %d\n", last_pool, proc_index);
    while (pp != 0) {
        for (int i = 0; i < NPROC; i++) {
            printf("%s", states[pp->proc_lists[i].proc.state]);
        }
        printf("\n");
        pp = pp->next;
    }
}