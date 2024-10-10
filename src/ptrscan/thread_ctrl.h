#ifndef THREAD_CTRL_H
#define THREAD_CTRL_H

#include <vector>

#include <cstdint>

#include <pthread.h>
#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "thread.h"
#include "mem_tree.h"
#include "mem.h"


//thread controller, 'global state' across threads
class thread_ctrl {

    private:
        //attributes
        std::vector<parent_range> parent_ranges;
        std::vector<thread> threads;
        
        pthread_barrier_t depth_barrier; 
        unsigned int current_depth;

    private:
        //methods
        uintptr_t get_rw_mem_sum(mem * m);
        void divide_mem(args_struct * args, mem * m, uintptr_t mem_sum);

    public:
        //methods
        void init(args_struct * args, mem * m, 
                  mem_tree * m_tree, ui_base * ui, pid_t pid); 

        void prepare_level(args_struct * args, mem * m, mem_tree * m_tree);
        void start_level();
        void end_level();
        void wait_thread_terminate();

};


#endif
