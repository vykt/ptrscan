#ifndef THREAD_CTRL_H
#define THREAD_CTRL_H

#include <vector>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "args.h"
#include "thread.h"
#include "mem_tree.h"
#include "proc_mem.h"


//thread controller, 'global state'
class thread_ctrl {

    //attributes
    public:
    std::vector<parent_range> parent_range_vector;
    std::vector<thread> thread_vector;
    pthread_barrier_t level_barrier;
    unsigned int current_level;

    //methods
    private:
    uintptr_t get_rw_mem_sum(proc_mem * p_mem);
    void define_regions_to_scan(args_struct * args, proc_mem * p_mem, 
                                uintptr_t mem_sum);

    public:
    void init(args_struct * args, proc_mem * p_mem, mem_tree * m_tree, pid_t pid); 

    void prepare_level(args_struct * args, proc_mem * p_mem, mem_tree * m_tree);
    void start_level();
    void end_level();

    void wait_thread_terminate();

};


#endif
