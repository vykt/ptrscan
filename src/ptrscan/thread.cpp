#include <vector>
#include <stdexcept>
#include <algorithm>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>

#include <unistd.h>

#include <pthread.h>
#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "thread.h"
#include "mem_tree.h"


static int _good_ret = 0;
static int _bad_ret = -1;


// --- EXTERNAL

//bootstrap function
void * thread_bootstrap(void * arg) {

    //cast to correct type
    thread_arg * cast_arg = (thread_arg *) arg;

    //call thread_main & release session on return
    try {
        cast_arg->t->thread_main(cast_arg->args, cast_arg->m, 
                                 cast_arg->m_tree, cast_arg->ui);
        cast_arg->t->release_session();

    } catch (std::runtime_error& e) {
        cast_arg->ui->report_exception(e); //TODO segfault?
        pthread_exit((void *) &_bad_ret);
    }

    //free arguments structure on exit
    delete((thread_arg *) arg);

    //exit
    pthread_exit((void *) &_good_ret);
}


// --- PRIVATE METHODS

//read the next buffer in a way that still scans for pointers across buffer and 
//region boundaries
ssize_t inline thread::get_next_buffer_smart(const args_struct * args, 
                                             read_state * r_state, 
                                             cm_byte * read_buf, 
                                             const uintptr_t read_addr, 
                                             const bool region_first_read) {

    const char * exception_str[1] = {
        "thread -> get_next_buffer_smart: memory read into buffer failed."
    };

    int ret;

    ssize_t min, max;
    ssize_t read_final_size, read_buf_effective_size;


    //if first read of this region, read the entire buffer
    if (region_first_read) {
        
        read_buf_effective_size = READ_BUF_SIZE;
        memset(read_buf, 0, read_buf_effective_size);
    
    //else copy end of last buffer to the start of next buffer
    } else {

        read_buf_effective_size = READ_BUF_SIZE - args->bit_width;
        
        memcpy(read_buf, read_buf+(r_state->last - args->bit_width), args->bit_width);
        memset(read_buf + args->bit_width, 0, read_buf_effective_size);
    }
     
    //get read target size
    min = 0;
    max = read_buf_effective_size;
    read_final_size = std::clamp(r_state->left, min, max);

    ret = ln_read(&this->session, read_addr, read_buf, read_final_size);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return read_final_size;
}



// --- PUBLIC METHODS

//compare current address to parent nodes
int thread::addr_parent_compare(const args_struct * args, const uintptr_t addr) {

    bool eval_range;    

    //check if preset offsets are in use
    if (args->use_preset_offsets) {
        
        //check if a preset offset was supplied for this depth level
        if ( (long int) args->preset_offsets[*this->current_depth - 1] != -1) {

            //for every parent region
            for (int i = 0; i < this->parent_ranges->size(); ++i) {

                eval_range = (addr == (*this->parent_ranges)[i].end_addr
                              - args->preset_offsets[*this->current_depth - 1]);
                if (eval_range) return i;
            } //end for
            
            //no match found, return -1 as index
            return -1;
        }
    }

    //otherwise, accept any offset under args->max_struct_size
    
    //for every parent region
    for (int i = 0; i < this->parent_ranges->size(); ++i) {

        eval_range = addr >= (*this->parent_ranges)[i].start_addr
                     && addr <= (*this->parent_ranges)[i].end_addr;

        if (eval_range) return i;
    } //end for
    
    //no match found, return -1 as index
    return -1;
}


//main loop for each thread
void thread::thread_main(const args_struct * args, const mem * m, 
                         mem_tree * m_tree, ui_base * ui) {

    const char * exception_str[1] = {
        "thread -> thread_main: memory read into local buffer failed."
    };

    int ret, parent_index;
    uintptr_t read_addr, potential_ptr_addr;

    read_state r_state;
    
    bool region_first_read;

    cm_byte read_buf[READ_BUF_SIZE];
    unsigned int buffer_increment;


    //set buffer_increment to aligned (sizeof(uintptr_t)) or unaligned (1)
    if (args->aligned) {
        buffer_increment = args->bit_width;
    } else {
        buffer_increment = 1;
    }


    //for every level in the tree (start at 1, 0 is root)
    for (unsigned int i = 1; i < args->max_depth; ++i) {

        //wait for thread_ctrl->start_level()
        pthread_barrier_wait(this->depth_barrier);

        //for every range this thread needs to scan
        for (int j = 0; j < this->vma_scan_ranges.size(); ++j) {

            //setup read state
            r_state.done = r_state.last = 0;
            r_state.left = r_state.total = this->vma_scan_ranges[j].end_addr
                                           - this->vma_scan_ranges[j].start_addr;

            //reset misc variables
            read_addr     = this->vma_scan_ranges[j].start_addr;
            region_first_read = true;

            //read while not done with region
            while (r_state.left != 0) {

                //get the next buffer
                r_state.last = this->get_next_buffer_smart(args, &r_state, 
                                                           read_buf, read_addr, 
                                                           region_first_read);
                r_state.left -= r_state.last;
                r_state.done += r_state.last;

                //for every aligned / unaligned pointer
                for (int k = region_first_read ? 0 : buffer_increment; 
                     k < r_state.last; k += buffer_increment) {

                    //convert value to ptr
                    potential_ptr_addr = *((uintptr_t *) (read_buf + k));
                    
                    //see if ptr points to any parent
                    parent_index = this->addr_parent_compare(args, potential_ptr_addr);

                    //if no match found, continue to next pointer
                    if (parent_index == -1) continue;

                    //otherwise, add new node
                    m_tree->add_node(read_addr + k, potential_ptr_addr, 
                                     this->vma_scan_ranges[j].vma_node,
                                     (*this->parent_ranges)[parent_index].parent_node,
                                     i, m);

                } //end for every aligned / unaligned pointer
 
                //increment next read address
                read_addr += r_state.last;

                //unset first read
                region_first_read = false;

            } //end while


            //report progress for thread
            if (args->verbose) {
                ui->report_thread_progress(j, 
                    (unsigned int) this->vma_scan_ranges.size(), this->ui_id);
            }

        } //end for every region


        //wait for thread_ctrl->end_level()
        pthread_barrier_wait(this->depth_barrier);

    } //end for every level
}


//reset current_addr (no race condition possible)
void thread::reset_current_addr() {

    this->current_addr = this->vma_scan_ranges[0].start_addr;

}


void thread::link_thread(const int ui_id, unsigned int * current_depth, 
                         pthread_barrier_t * depth_barrier, 
                         std::vector<parent_range> * parent_ranges) {

    this->ui_id         = ui_id;
    this->current_depth = current_depth;
    this->depth_barrier = depth_barrier;
    this->parent_ranges = parent_ranges;

    return;
}


void thread::setup_session(const int ln_iface, const pid_t pid) {

    const char * exception_str[1] = {
        "thread -> setup_session: failed to open a liblain session."
    };

    int ret;

    ret = ln_open(&this->session, ln_iface, pid);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


void thread::release_session() {

    const char * exception_str[1] = {
        "thread -> release_session: failed to release a liblain session."
    };

    int ret;

    ret = ln_close(&this->session);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


inline pthread_t * thread::get_id() {
    return &this->id;
}


inline const int thread::get_ui_id() const {
    return this->ui_id;
}


inline void thread::set_ui_id(int ui_id) {
    this->ui_id = ui_id;
    return;
}


inline const std::vector<vma_scan_range> * thread::get_vma_scan_ranges() const {
    return &this->vma_scan_ranges;
}


inline void thread::add_vma_scan_range(vma_scan_range range) {
    this->vma_scan_ranges.push_back(range);
    return;
}
