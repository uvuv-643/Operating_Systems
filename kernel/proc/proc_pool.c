#include "../types.h"
#include "../param.h"
#include "../memlayout.h"
#include "../riscv.h"
#include "../spinlock.h"
#include "../defs.h"
#include "proc.h"

struct memory_pool {
    struct proc procs[NPROC];
    struct memory_pool* prev;
    struct memory_pool* next;
};

int last_pool = 0;
int proc_index = 0;

// how often after free system has to find unused pools
int proc_interval = NPROC / 2;
int proc_interval_current = 0;
int min_active_proc_count_in_pool = NPROC / 2;

struct memory_pool* head;
struct spinlock pool_lock;
struct spinlock interval_lock;


// init base memory pool for all the processed
// function must be called once when the proccess module starts
// no lock has to be acquired
void init_memory_pool() {
    initlock(&pool_lock, "pool_lock");
    initlock(&interval_lock, "interval_lock");
    head = (struct memory_pool* )malloc(sizeof(struct memory_pool));
    memset(head, 0, sizeof(struct memory_pool));
}


// if there are many pools in system, the function frees unused pools
// pool is unused when all the processes have state "UNUSED"
// no lock has to be acquired before entering function
void free_unused_memory_pools() {

    acquire(&pool_lock);

    // that is pools which will be merged (?) by this function
    struct memory_pool* first_small_pool = 0;
    int first_small_pool_cnt = 0;
    struct memory_pool* second_small_pool = 0;
    int second_small_pool_cnt = 0;

    struct memory_pool* current_pool = head->next;

    // there we find two smallest pools for merging
    // if merge is possible
    // first_small_pool != 0 && second_small_pool != 0
    while (current_pool != 0) {
        int number_of_used_procs = 0;
        for (int i = 0; i < NPROC; i++) {
            if (current_pool->procs->state != UNUSED) {
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
    if (first_small_pool != 0 && second_small_pool != 0) {
        struct memory_pool* pool = (struct memory_pool*) malloc(sizeof(struct memory_pool));
        memset(head, 0, sizeof(struct memory_pool));
        int l = 0; int r = 0;
        while (l < first_small_pool_cnt && r < second_small_pool_cnt) {
            if (l < first_small_pool_cnt && r < second_small_pool_cnt) {
                if (first_small_pool->procs[l].pid < second_small_pool->procs[r].pid) {
                    pool->procs[l + r] = first_small_pool->procs[l];
                    l++;
                } else {
                    pool->procs[l + r] = second_small_pool->procs[r];
                    r++;
                }
            }
        }
    }
    
    proc_interval_current = 0;
    release(&pool_lock);
}


void extend_pool_if_needed() {

}


struct proc* take_from_pool() {



    return 0;
}

void free_from_pool(struct proc* proc) {
    acquire(&proc->lock);
    proc->state = UNUSED;
    release(&proc->lock);
    acquire(&interval_lock);
    proc_interval_current++;
    if (proc_interval_current >= proc_interval) {
        free_unused_memory_pools();
        proc_interval_current = 0;
    }
    release(&interval_lock);

}