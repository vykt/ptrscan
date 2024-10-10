#ifndef UI_BASE_H
#define UI_BASE_H

#include <vector>
#include <string>

#include <libcmore.h>
#include <liblain.h>


//abstract ui class, inherited from by terminal and tui ncurses interfaces
class ui_base {

    //methods
    public:
    virtual void report_exception(const std::exception& e) = 0;
    virtual void report_depth_progress(int depth_done) = 0;
    virtual void report_thread_progress(unsigned int vma_done,
                                        unsigned int vma_total,
                                        int human_thread_id) = 0;
    
    virtual pid_t clarify_pid(cm_vector * pids) = 0;
    //passing void * instead of serialise * to solve circular header include
    virtual void output_serialised_results(void * args_ptr,
                                           void * serialise_ptr, void * mem_ptr) = 0;
};


#endif
