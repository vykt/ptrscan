#include <vector>
#include <list>
#include <stdexcept>
#include <algorithm>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "thread_ctrl.h"
#include "thread.h"
#include "mem_tree.h"
#include "mem.h"


// --- PRIVATE METHODS

//get total rw- memory
const uintptr_t thread_ctrl::get_rw_mem_sum(const mem * m) {

    uintptr_t mem_sum, region_size;

    ln_vm_area * vma;
    const std::vector<cm_list_node *> * rw_areas;

    mem_sum = 0;
    rw_areas = m->get_rw_areas();

    //for every rw- vma
    for (int i = 0; i < rw_areas->size(); ++i) {
        
        vma = LN_GET_NODE_AREA((*rw_areas)[i]);
        region_size = vma->end_addr - vma->start_addr;
        
        mem_sum += region_size;
    
    } //end for

    return mem_sum;
}



//define the regions a thread must scan
void thread_ctrl::divide_mem(const args_struct * args, 
                             const mem * m, const uintptr_t mem_sum) {
    
    int region_index;
    uintptr_t mem_left, mem_share, temp_share;
    uintptr_t region_progress, region_size, region_left;
    uintptr_t fwd_min, fwd_max;

    vma_scan_range temp_range;

    const std::vector<cm_list_node *> * rw_areas;
    ln_vm_area * vma;

    //assign shares
    mem_left = mem_sum;
    mem_share = mem_left / args->threads;
    region_index = region_progress = region_size = region_left = 0;
    fwd_min = 0;
    fwd_max = args->bit_width;


    rw_areas = m->get_rw_areas();

    //for every thread
    for (unsigned int i = 0; i < args->threads; ++i) {

        //check if this is the last thread, it gets the rest of the remaining memory
        if (i == args->threads - 1) {
            temp_share = mem_left;
        //otherwise assign mem_share worth of memory
        } else {
            temp_share = mem_share;
        }

        //while there is still vmas that needs to be assigned to the current thread
        while (temp_share != 0) {

            vma = LN_GET_NODE_AREA((*rw_areas)[region_index]);

            //zero out the mem_range temp buffer
            memset(&temp_range, 0, sizeof(temp_range));

            //get current region's size & last thread's progress through it
            region_size = vma->end_addr - vma->start_addr;
            region_left = region_size - region_progress;
            
            //if remainder of region is greater than what is left of this thread share
            if (region_left > temp_share) {
                
                //bring up temp_share to a region boundary
                if (temp_share % args->bit_width != 0) {
                    temp_share += args->bit_width - (temp_share % args->bit_width);
                }

                //create new vma_scan_range entry for current thread
                temp_range.vma_node   = (*rw_areas)[region_index];
                temp_range.start_addr = vma->start_addr + region_progress;
                temp_range.end_addr   = temp_range.start_addr + temp_share;

                //record progress through current region
                region_progress += temp_share;

                //get bytes left in this region
                region_left = vma->end_addr - temp_range.end_addr;

                //check if incrementing temp_share to reach a pointer boundary makes 
                //it reach the end of the current region
                if (!region_left) {
                    region_index++;
                    region_progress = 0;
                //otherwise let this thread region scan up to bit_width bytes
                //ahead to scan gaps between thread shares
                } else {
                    temp_range.end_addr += std::clamp(region_left, fwd_min, fwd_max);
                }

                //add scan region to thread
                this->threads[i].add_vma_scan_range(temp_range);
                mem_left -= temp_share;

                //set temp_share to 0 to continue onto the next thread share
                temp_share = 0;

            //otherwise region fits in thread share
            } else {
                
                //reduce temp_share by region_size
                temp_share -= region_left;

                //create new mem_range entry for current thread
                temp_range.vma_node = (*rw_areas)[region_index];
                temp_range.start_addr = vma->start_addr + region_progress;
                temp_range.end_addr = vma->end_addr;

                //add scan region to thread
                this->threads[i].add_vma_scan_range(temp_range);
                mem_left -= region_left;

                //move onto next region
                region_progress = 0;
                region_index++;

           } //end if-else
        } //end while
    } //end for every thread
}


// --- PUBLIC METHODS

//initialise thread controller & spawn threads
thread_ctrl::thread_ctrl(const args_struct * args, const mem * m, 
                         const mem_tree * m_tree, ui_base * ui) {

    const char * exception_str[3] = { 
        "thread_ctrl -> init: pthread_barrier_init() failed.",
        "thread_ctrl -> init: failed to open a liblain session for this thread.",
        "thread_ctrl -> init: pthread_create() failed."
    };

    int ret;
    int next_ui_id;

    //get total amount of rw-/rwx memory
    const uintptr_t mem_sum = get_rw_mem_sum(m);

    thread_arg * t_arg;
    thread t_temp;

    //setup human readable thread ids
    next_ui_id = 1;

    //set current level to 0 (root node)
    this->current_depth = 0;

    //initialise thread synchronisation, +1 to include main thread
    ret = pthread_barrier_init(&this->depth_barrier, nullptr, args->threads+1);
    if (ret != 0) {
        throw std::runtime_error(exception_str[0]);
    }

    //instantiate thread objects
    for (unsigned int i = 0; i < args->threads; ++i) {
       
        //set human readable thread id
        t_temp.set_ui_id(next_ui_id);
        next_ui_id++;

        //link thread to thread_ctrl
        t_temp.link_thread(next_ui_id, &this->current_depth,
                           &this->depth_barrier, &this->parent_ranges);
        
        //open a liblain session for this thread
        t_temp.setup_session(args->ln_iface, m->get_pid());
        if (ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }

        //push new thread object
        this->threads.push_back(t_temp);
    }

    //define memory regions each thread will scan
    divide_mem(args, m, mem_sum);

    //spawn each thread
    for (int i = 0; i < this->threads.size(); ++i) { 

        //setup args structure for new thread (deallocated by thread on exit)
        t_arg = new(thread_arg);

        t_arg->t = &this->threads[i];
        t_arg->args = (args_struct *) args;
        t_arg->m = (mem *) m;
        t_arg->m_tree = (mem_tree *) m_tree;
        t_arg->ui = ui;

        //create thread
        ret = pthread_create(this->threads[i].get_id(), nullptr,
                             &thread_bootstrap, (void *) t_arg);
        if (ret != 0) {
            throw std::runtime_error(exception_str[2]);
        }
    }
}


//setup threads for next iteration
void thread_ctrl::prepare_threads(args_struct * args, mem * m, mem_tree * m_tree) {

    const char * exception_str[1] = {
        "thread_ctrl -> prepare_level: ln_get_area_offset returned -1. Impossible?"
    };

    int ret;
    unsigned int min, max, off;
    uintptr_t allowed_struct_size;
    
    cm_list_node * vma_node;
    ln_vm_area * vma;

    parent_range temp_parent_range;
    std::list<mem_node *> * level_list;


    //increment current level
    this->current_depth += 1;

    //reset current_addr for each thread
    for (int i = 0; i < this->threads.size(); ++i) {
        this->threads[i].reset_current_addr();
    }

    //erase parent_range_vector
    this->parent_ranges.erase(parent_ranges.begin(), parent_ranges.end());

    
    //re-create parent_range_vector for this level
    
    //get the list for the current level
    level_list = m_tree->get_level_list(this->current_depth - 1);

    //for every member of current level list
    for (std::list<mem_node *>::iterator it = level_list->begin();
         it != level_list->end(); ++it) {

        //assign address of this node
        temp_parent_range.end_addr = (*it)->get_addr();

        vma_node = (cm_list_node *) (*it)->get_vma_node();
        vma = LN_GET_NODE_AREA(vma_node);

        off = (unsigned int)ln_get_area_offset(vma_node, temp_parent_range.end_addr);
        if (off == -1) {
            throw std::runtime_error(exception_str[0]);
        }

        //calculate permitted lookback
        min = 0;
        max = (unsigned int) args->max_struct_size;
        allowed_struct_size = std::clamp(off, min, max);

        //assign start addr based on allowed lookback
        temp_parent_range.start_addr = temp_parent_range.end_addr
                                       - allowed_struct_size;
        
        //assign node pointer
        temp_parent_range.parent_node = (*it);

        //add temp_parent_range to this level's list of parent ranges
        this->parent_ranges.push_back(temp_parent_range);
    
    } //end for

    return;
}


//start the level next level
void thread_ctrl::start_level() {

    const char * exception_str[1] {
        "thread_ctrl -> start_level: pthread_barrier_wait() returned an error."
    };

    int ret;

    //wait on barrier for threads to become ready
    ret = pthread_barrier_wait(&this->depth_barrier);
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
    ret = pthread_barrier_wait(&this->depth_barrier);
    if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
        throw std::runtime_error(exception_str[0]);
    }
}


//wait for all threads to terminate
thread_ctrl::~thread_ctrl() {

    const char * exception_str[2] {
        "thread_ctrl -> wait_thread_terminate: pthread_join() failed to join.",
        "thread_ctrl -> wait_thread_terminate: pthread_join() thread returned error."
    };

    int ret;
    int * thread_ret;

    //for every thread
    for (int i = 0; i < this->threads.size(); ++i) {
        ret = pthread_join(*this->threads[i].get_id(), (void **) &thread_ret);
        if (ret != 0) {
            throw std::runtime_error(exception_str[0]);
        }
        if (*thread_ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }
    } //end for

    return;
}
