
struct proc_pool {
    struct proc_list proc_lists[NPROC];
    struct proc_pool* prev;
    struct proc_pool* next;
};


