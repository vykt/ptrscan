#include <vector>

#include <cstdint>

#include <pthread.h>

#include "args.h"
#include "thread.h"
#include "mem_tree.h"



//bootstrap function
void * thread_bootstrap(void * arg_bootstrap) {

    //cast to correct type
    thread_arg * real_arg = (thread_arg *) arg_bootstrap;

    //call thread_main
    real_arg->t->thread_main(real_arg->args);

    return nullptr;
}


//thread class

//thread constructor
thread::thread(pthread_barrier_t * level_barrier,
               std::vector<parent_range> * parent_range_vector) {

    this->level_barrier = level_barrier;
    this->parent_range_vector = parent_range_vector;

}


//main loop for each thread
int thread::thread_main(args_struct * args) {

    //for every level in the tree (start at 1, 0 is root)
    for (int i = 1; i < args->levels; ++i) {

        //wait for thread_ctrl->start_level()
        pthread_barrier_wait(this->level_barrier);


        /*  1) GET BUFFER FROM REGIONS_TO_SCAN
         *
         *  2) --- GET NEXT POINTER
         *
         *  3) --- COMPARE AGAINST PARENT_RANGE_VECTOR
         *
         *  4) --- IF MATCHING
         *             >CALL add_node()
         *  
         *  
         *
         *
         *
         *
         *
         *
         *
         *
         *
         */
        

        //wait for thread_ctrl->end_level()
        pthread_barrier_wait(this->level_barrier);

    } //end for

    return 0;
}



//functions for use by thread_ctrl

//reset current_addr (no race condition possible)
void thread::reset_current_addr() {

    this->current_addr = this->regions_to_scan[0].start_addr;

}


//check if thread finished the level - 0:success, -1:fail
int thread::confirm_finished_level() {

    //if finished, reset finished_level to false
    if (this->finished_level) {
        this->finished_level = false;
        return 0;
    }

    return -1;
}
