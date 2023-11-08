#include "../types.h"
#include "../param.h"
#include "../memlayout.h"
#include "../riscv.h"
#include "../spinlock.h"
#include "../defs.h"
#include "proc.h"

struct memory_pool {
    struct proc procs[NPROC];
    struct memory_pool* next;
};

int last_pool = 0;
int proc_index = 0;

// how often after free system has to find unused pools
int proc_interval = NPROC / 2;
int proc_interval_current = 0;

struct memory_pool* head;
struct spinlock pool_lock;
struct spinlock interval_lock;


// init base memory pool for all the processed
// function must be called once when the proccess module starts
// no lock has to be acquired
void init_memory_pool() {
    head = (struct memory_pool* )malloc(sizeof(struct memory_pool));
    initlock(&pool_lock, "pool_lock");
    initlock(&interval_lock, "interval_lock");
    memset(head, 0, sizeof(struct memory_pool));
}


// if there are many pools in system, the function frees unused pools
// pool is unused when all the processes have state "UNUSED"
// no lock has to be acquired before entering function
void free_unused_memory_pools() {
    acquire(&pool_lock);
    struct memory_pool* current_pool = head;
    while (current_pool != 0 && current_pool->next != 0) {
        struct memory_pool* next_pool = current_pool->next;
        int number_of_used_procs = 0;
        for (int i = 0; i < NPROC; i++) {
            if (next_pool->procs->state != UNUSED) {
                number_of_used_procs++;
                break;
            }
        }
        if (!number_of_used_procs) {
            current_pool->next = 0;
            struct memory_pool* have_to_be_removed = next_pool;
            struct memory_pool* mpp = have_to_be_removed;
            while (mpp != 0) {
                mpp = mpp->next;
                free(have_to_be_removed);
                have_to_be_removed = mpp;
            }
            return;  
        }
        current_pool = current_pool->next;
    }
    release(&pool_lock);
}


void extend_pool_if_needed() {

}


struct proc* take_from_pool() {



    return 0;
}

void free_from_pool() {

    acquire(&interval_lock);
    proc_interval_current++;
    if (proc_interval_current >= proc_interval) {
        free_unused_memory_pools();
        proc_interval_current = 0;
    }
    release(&interval_lock);

}