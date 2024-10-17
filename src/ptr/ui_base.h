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
    virtual void report_depth_progress(const int depth_done) = 0;
    virtual void report_thread_progress(const unsigned int vma_done,
                                        const unsigned int vma_total,
                                        const int human_thread_id) = 0;
    
    //passing void * instead of serialise * to solve circular header include
    virtual void output_ptrchains(const void * args_ptr,
                                  const void * s_ptr, const void * m_ptr) = 0;
};


#endif
