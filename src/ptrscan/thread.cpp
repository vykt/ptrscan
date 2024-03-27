#include <vector>
#include <stdexcept>
#include <algorithm>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <unistd.h>

#include <libpwu.h>
#include <pthread.h>

#include "args.h"
#include "thread.h"
#include "mem_tree.h"



//bootstrap function
void * thread_bootstrap(void * arg_bootstrap) {

    int good_ret = 0;
    int bad_ret = -1;

    //cast to correct type
    thread_arg * real_arg = (thread_arg *) arg_bootstrap;

    //call thread_main
    try {
        real_arg->t->thread_main(real_arg->args, real_arg->p_mem, real_arg->m_tree,
                                 real_arg->ui);
    } catch (std::runtime_error& e) {
        real_arg->ui->report_exception(e); //TODO this will cause a segfault if it runs
        pthread_exit((void *) &bad_ret);
    }

    //close descriptor
    close(real_arg->t->mem_fd);

    //free arguments structure on exit
    free(arg_bootstrap);

    //exit
    pthread_exit((void *) &good_ret);
}


/*
 *  the efficiency of reading can probably be improved with a circular buffer
 */

//read the next buffer while allowing for unaligned pointer scanning across 
//buffer boundaries
ssize_t inline thread::get_next_buffer_smart(byte * mem_buf, ssize_t read_left, 
                                            ssize_t read_last, bool first_region_read) {

    const char * exception_str[2] = {
        "thread -> get_next_buffer_smart: memory read into buffer failed.\0"
        "thread -> get_next_buffer_smart: failsafe memory read into buffer failed.\0"
    };

    ssize_t min, max;
    ssize_t read_bytes, read_bytes_failsafe, to_read, read_buf_effective_size;

    //if first read of this region, read the entire buffer
    if (first_region_read) {
        read_buf_effective_size = READ_BUF_SIZE;
        memset(mem_buf, 0, read_buf_effective_size);
    //else copy end of last buffer to the start of next buffer
    } else {
        read_buf_effective_size = READ_BUF_SIZE - sizeof(uintptr_t);
        memcpy(mem_buf, mem_buf+(read_last - sizeof(uintptr_t)), 
               sizeof(uintptr_t));
        memset(mem_buf + sizeof(uintptr_t), 0, read_buf_effective_size);
    }
     
    //get read target size
    min = 0;
    max = read_buf_effective_size;
    to_read = std::clamp(read_left, min, max);

    //read up to read_buf_effective_size bytes (if scheduler is kind)
    read_bytes = read(this->mem_fd, mem_buf, to_read);
    if (read_bytes == -1) {
        throw std::runtime_error(exception_str[0]);
    }

    /*
     *  From my testing, calling read() on /proc/<pid>/mem should always return the 
     *  full requested amount up to requests of around 0x16000. As such, this next 
     *  call should effectively never take place. Nontheless, it is here as a 
     *  failsafe.
     *
     *  Ensuring the read buffer is sizeof(uintptr_t) aligned simplies everything.
     */

    //call read again to align buffer if necessary
    if (read_bytes % sizeof(uintptr_t)) {
        read_bytes_failsafe = read(this->mem_fd, mem_buf+read_bytes,
                                   read_bytes % sizeof(uintptr_t));
        if (read_bytes_failsafe == -1) {
            throw std::runtime_error(exception_str[1]);
        }
        read_bytes += read_bytes_failsafe;
    }

    return read_bytes;
}



//thread class

//compare current address to parent nodes
int thread::addr_parent_compare(uintptr_t addr, args_struct * args) {

    bool eval_range;

    //for every parent region
    for (unsigned int i = 0; i < (unsigned int) this->parent_range_vector->size(); ++i) {

        //evaluate if addr falls in the range of this parent node
        eval_range = addr >= (*this->parent_range_vector)[i].start_addr
                     && addr <= (*this->parent_range_vector)[i].end_addr;

        //if match found, return index into this->parent_range_vector
        if (eval_range) {
            return i;
        }

    } //end for every parent region
    
    //if no match found, return -1 as index
    return -1;
}


//main loop for each thread
void thread::thread_main(args_struct * args, proc_mem * p_mem, mem_tree * m_tree,
                         ui_base * ui) {

    const char * exception_str[2] = {
        "thread -> thread_main: failed to seek to start of region.\0",
        "thread -> thread_main: memory read into local buffer failed.\0"
    };

    int ret, parent_index;

    ssize_t read_left, read_last, read_total;

    uintptr_t read_addr, potential_ptr_addr;
    bool first_region_read;

    byte mem_buf[READ_BUF_SIZE];
    unsigned int buffer_increment;


    //set buffer_increment to aligned (sizeof(uintptr_t)) or unaligned (1)
    if (args->aligned) {
        buffer_increment = sizeof(uintptr_t);
    } else {
        buffer_increment = 1;
    }


    //for every level in the tree (start at 1, 0 is root)
    for (unsigned int i = 1; i < args->levels; ++i) {

        //wait for thread_ctrl->start_level()
        pthread_barrier_wait(this->level_barrier);

        //for every region this thread needs to scan
        for (unsigned int j = 0; j < (unsigned int) this->regions_to_scan.size(); ++j) {

            /*
             *  not reading with libpwu to avoid calling lseek
             */

            //seek to start of region
            ret = (off_t) lseek(this->mem_fd, this->regions_to_scan[j].start_addr,
                                SEEK_SET);
            if (ret == -1) {
                throw std::runtime_error(exception_str[0]);
            }

            //get size of region
            read_left = read_total = this->regions_to_scan[j].end_addr
                                     - this->regions_to_scan[j].start_addr;

            //reset misc variables
            read_addr = this->regions_to_scan[j].start_addr;
            first_region_read = true;
            read_last = 0;

            //read while not done with region
            while (read_left != 0) {

                //get the next buffer
                read_last = this->get_next_buffer_smart(mem_buf, read_left, read_last, 
                                                        first_region_read);
                read_left -= read_last;

                //for every aligned / unaligned pointer
                for (int k = 0; k < read_last; k += buffer_increment) {

                    //convert value to ptr
                    potential_ptr_addr = *((uintptr_t *) (mem_buf + k));
                    
                    //see if ptr points to any parent
                    parent_index = this->addr_parent_compare(potential_ptr_addr,
                                                             args);

                    //if no match found, continue to next pointer
                    if (parent_index == -1) continue;

                    //otherwise, add new node
                    m_tree->add_node(
                            read_addr + k,
                            potential_ptr_addr,
                            (*this->parent_range_vector)[parent_index].parent_node,
                            i,
                            p_mem);

                } //end for every aligned / unaligned pointer
 
                //increment current address accordingly
                if (first_region_read) {
                    first_region_read = false;
                }
                read_addr += read_last;

            } //end while


            //report progress for thread
            if (args->verbose) {
                ui->report_thread_progress(j, 
                    (unsigned int) this->regions_to_scan.size(),
                    this->human_thread_id);
            }

        } //end for every region


        //wait for thread_ctrl->end_level()
        pthread_barrier_wait(this->level_barrier);

    } //end for every level
}



//functions for use by thread_ctrl

//reset current_addr (no race condition possible)
void thread::reset_current_addr() {

    this->current_addr = this->regions_to_scan[0].start_addr;

}
