#include <vector>
#include <list>
#include <stdexcept>
#include <algorithm>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <libpwu.h>

#include "args.h"
#include "thread_ctrl.h"
#include "thread.h"
#include "mem_tree.h"
#include "proc_mem.h"


// --- private

//get total rw- memory
uintptr_t thread_ctrl::get_rw_mem_sum(proc_mem * p_mem) {

    uintptr_t mem_sum, region_size;

    mem_sum = 0;

    //for every rw- memory segment
    for (unsigned int i = 0; i < (unsigned int) p_mem->rw_regions_vector.size(); ++i) {
        region_size = (uintptr_t) p_mem->rw_regions_vector[i]->end_addr 
                  - (uintptr_t) p_mem->rw_regions_vector[i]->start_addr;
        mem_sum += region_size;
    } //end for

    return mem_sum;
}



//define the regions a thread must scan
void thread_ctrl::define_regions_to_scan(args_struct * args, proc_mem * p_mem, 
                                         uintptr_t mem_sum) {
    
    int reg_ind;
    uintptr_t mem_share, temp_share;
    uintptr_t region_progress, region_size, region_left;
    uintptr_t fwd_min, fwd_max;

    mem_range temp_range;

    //assign shares
    mem_share = mem_sum / args->num_threads;
    reg_ind = region_progress = region_size = region_left = 0;
    fwd_min = 0;
    fwd_max = sizeof(uintptr_t);

    //for every thread
    for (unsigned int i = 0; i < args->num_threads; ++i) {
        
        //check if this is the last thread, it gets the rest of the remaining memory
        if (i == args->num_threads - 1) {
            temp_share = mem_sum;
        //otherwise assign mem_share worth of memory
        } else {
            temp_share = mem_share;
        }

        //while there is still memory that needs to be assigned to the current thread
        while (temp_share != 0) {

            //zero out the mem_range temp buffer
            memset(&temp_range, 0, sizeof(temp_range));

            //get current memory region size & last thread share's progress through it
            region_size = (uintptr_t) p_mem->rw_regions_vector[reg_ind]->end_addr
                          - (uintptr_t) p_mem->rw_regions_vector[reg_ind]->start_addr;
            region_left = region_size - region_progress;
            
            //if remainder of region is greater than what is left of this thread share
            if (region_left > temp_share) {
                
                //bring up temp_share to a ptr boundary, this should NOT segfault
                if (temp_share % sizeof(uintptr_t) != 0) {
                    temp_share += sizeof(uintptr_t) 
                    - (temp_share % sizeof(uintptr_t));
                }

                //create new mem_range entry for current thread
                temp_range.m_entry = p_mem->rw_regions_vector[reg_ind];
                temp_range.start_addr = (uintptr_t) 
                                        p_mem->rw_regions_vector[reg_ind]->start_addr
                                        + region_progress;
                temp_range.end_addr = temp_range.start_addr + temp_share;

                //record progress through current region
                region_progress += temp_share;

                //get bytes left in this region
                region_left = (uintptr_t) temp_range.m_entry->end_addr
                              - temp_range.end_addr;

                //check if incrementing temp_share to reach a pointer boundary makes 
                //it reach the end of the current region
                if (!region_left) {
                    reg_ind++;
                    region_progress = 0;
                //otherwise let this thread region scan up to sizeof(uintptr_t) bytes
                //ahead to scan gaps between thread shares during unaligned scans
                } else {
                    temp_range.end_addr += std::clamp(region_left, fwd_min, fwd_max);
                }

                //add region to thread
                this->thread_vector[i].regions_to_scan.push_back(temp_range);
                mem_sum -= temp_share;

                //set temp_share to 0 to continue onto the next thread share
                temp_share = 0;

            //otherwise region fits in thread share
            } else {
                
                //reduce temp_share by region_size
                temp_share -= region_left;

                //create new mem_range entry for current thread
                temp_range.m_entry = p_mem->rw_regions_vector[reg_ind];
                temp_range.start_addr = (uintptr_t)
                                        p_mem->rw_regions_vector[reg_ind]->start_addr
                                        + region_progress;
                temp_range.end_addr = (uintptr_t)
                                      p_mem->rw_regions_vector[reg_ind]->end_addr;

                //add region to thread
                this->thread_vector[i].regions_to_scan.push_back(temp_range);
                mem_sum -= region_left;

                //move onto next region
                region_progress = 0;
                reg_ind++;

           } //end if-else
        } //end while
    } //end for every thread
}


// --- public

//initialise thread controller & spawn threads
void thread_ctrl::init(args_struct * args, proc_mem * p_mem, 
                       mem_tree * m_tree, ui_base * ui, pid_t pid) {

    const char * exception_str[4] = { 
        "thread_ctrl -> init: pthread_barrier_init() failed",
        "thread_ctrl -> init: failed to open fd on proc mem for thread",
        "thread_ctrl -> init: failed to malloc() thread arguments",
        "thread_ctrl -> init: pthread_create() failed"
    };

    int ret;
    int next_human_thread_id;
    uintptr_t mem_sum;

    thread_arg * t_arg;
    thread t_temp;


    //setup human readable thread ids
    next_human_thread_id = 1;

    //set current level to 0 (root node)
    this->current_level = 0;

    //initialise thread level barrier, +1 since main thread handles control
    ret = pthread_barrier_init(&this->level_barrier, NULL, args->num_threads+1);
    if (ret != 0) {
        throw std::runtime_error(exception_str[0]);
    }

    //get memory sum
    mem_sum = get_rw_mem_sum(p_mem);

    //instantiate thread objects
    for (unsigned int i = 0; i < args->num_threads; ++i) {
       
        //set human thread id
        t_temp.human_thread_id = next_human_thread_id;
        next_human_thread_id += 1;

        //setup links to controller
        t_temp.level_barrier = &this->level_barrier;
        t_temp.parent_range_vector = &this->parent_range_vector;
        
        //open file descriptor on proc mem for this thread
        ret = open_memory(pid, NULL, &t_temp.mem_fd);
        if (ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }

        //push new thread object
        this->thread_vector.push_back(t_temp);
    }

    //define memory regions each thread will scan
    define_regions_to_scan(args, p_mem, mem_sum);

    //spawn each thread
    for (unsigned int i = 0; i < this->thread_vector.size(); ++i) { 

        //setup args structure for new thread (deallocated by thread on exit)
        t_arg = (thread_arg *) malloc(sizeof(thread_arg));
        if (!t_arg) {
            throw std::runtime_error(exception_str[2]);
        }

        t_arg->args = args;
        t_arg->p_mem = p_mem;
        t_arg->m_tree = m_tree;
        t_arg->ui = ui;
        t_arg->t = &this->thread_vector[i];

        //create thread
        ret = pthread_create(&this->thread_vector[i].id, NULL,
                              &thread_bootstrap, (void *) t_arg);
        if (ret != 0) {
            throw std::runtime_error(exception_str[3]);
        }
    }
}


//setup threads for next level scan
void thread_ctrl::prepare_level(args_struct * args, proc_mem * p_mem, 
                                mem_tree * m_tree) {

    const char * exception_str[1] = {
        "thread_ctrl -> prepare_level: get_region_by_addr() didn't succeed for parent."
    };

    int ret;
    parent_range temp_parent_range;
    std::list<mem_node *> * level_list;

    uintptr_t allowed_lookback;
    maps_entry * matched_m_entry;
    unsigned int matched_m_offset;

    unsigned int min, max;



    //increment current level
    this->current_level += 1;

    //reset current_addr for each thread
    for (unsigned int i = 0; i < (unsigned int) this->thread_vector.size(); ++i) {
        this->thread_vector[i].reset_current_addr();
    }

    //erase parent_range_vector
    this->parent_range_vector.erase(parent_range_vector.begin(),
                                    parent_range_vector.end());

    
    //re-create parent_range_vector for this level
    
    //get the list for the current level
    level_list = &(*m_tree->levels)[this->current_level-1];

    //for every member of current level list
    for (std::list<mem_node *>::iterator it = level_list->begin();
         it != level_list->end(); ++it) {

        //assign address of this node
        temp_parent_range.end_addr = (*it)->node_addr;

        //calculate allowed lookback (shouldn't cross segment boundaries)
        ret = get_region_by_addr((void *) (*it)->node_addr, &matched_m_entry,
                                 &matched_m_offset, &p_mem->m_data);
        //throw exception if there is no match (should be never, memory corruption?)
        if (ret != 0) {
            throw std::runtime_error(exception_str[0]);
        }

        //calculate permitted lookback
        min = 0;
        max = (unsigned int) args->ptr_lookback;
        allowed_lookback = std::clamp(matched_m_offset, min, max);

        //assign start addr based on allowed lookback
        temp_parent_range.start_addr = temp_parent_range.end_addr - allowed_lookback;
        
        //assign node pointer
        temp_parent_range.parent_node = (*it);

        //add temp_parent_range to vector of parent vectors
        this->parent_range_vector.push_back(temp_parent_range);
    }
}


//start the level next level
void thread_ctrl::start_level() {

    const char * exception_str[1] {
        "thread_ctrl -> start_level: pthread_barrier_wait() returned an error."
    };

    int ret;

    //wait on barrier for threads to become ready
    ret = pthread_barrier_wait(&this->level_barrier);
    if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
        throw std::runtime_error(exception_str[0]);
    }
}


//end level
void thread_ctrl::end_level() {

    const char * exception_str[1] {
        "thread_ctrl -> end_level: pthread_barrier_wait() returned an error."
    };

    int ret;

    //wait on barrier for threads to finish current level
    ret = pthread_barrier_wait(&this->level_barrier);
    if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
        throw std::runtime_error(exception_str[0]);
    }
}


//wait for all threads to terminate
void thread_ctrl::wait_thread_terminate() {

    const char * exception_str[2] {
        "thread_ctrl -> wait_thread_terminate: pthread_join() failed to join threads.",
        "thread_ctrl -> wait_thread_terminate: pthread_join() thread returned bad_ret."
    };

    int ret;
    int * thread_ret;

    //for every thread
    for (unsigned int i = 0; i < (unsigned int) this->thread_vector.size(); ++i) {
        ret = pthread_join(this->thread_vector[i].id, (void **) &thread_ret);
        if (ret != 0) {
            throw std::runtime_error(exception_str[0]);
        }
        if (*thread_ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }
    } //end for 
}
