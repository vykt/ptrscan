#ifndef UI_BASE_H
#define UI_BASE_H

#include <vector>
#include <string>

#include <libpwu.h>


//util functions
int match_maps_obj(std::string basename, void * proc_mem_ptr, 
                   maps_obj ** matched_m_obj);


//abstract ui class, inherited from by terminal and tui ncurses interfaces
class ui_base {

    //methods
    public:
    virtual void report_exception(const std::exception& e) = 0;
    virtual void report_control_progress(int level_done) = 0;
    virtual void report_thread_progress(unsigned int region_done,
                                        unsigned int region_total,
                                        unsigned int current_level,
                                        int human_thread_id) = 0;
    
    virtual pid_t clarify_pid(name_pid * n_pid) = 0;
    //passing void * instead of serialise * to solve circular header include
    virtual void output_serialised_results(void * args_ptr,
                                           void * serialise_ptr,
                                           void * proc_mem_ptr) = 0;
};


#endif
