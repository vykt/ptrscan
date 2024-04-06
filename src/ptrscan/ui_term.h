#ifndef UI_TERM_H
#define UI_TERM_H

#include <vector>
#include <string>

#include <cstdint>

#include <libpwu.h>

#include "ui_base.h"


#define RESET "\033[0m"
#define RED   "\033[31m" //error (?)
#define GREEN "\033[32m" //static



//virtual ui class, inherited from by terminal and tui ncurses interfaces
class ui_term : public ui_base {

    public:
        virtual void report_exception(const std::exception& e);
        virtual void report_control_progress(int level_done);
        virtual void report_thread_progress(unsigned int region_done,
                                            unsigned int region_total,
                                            unsigned int current_level,
                                            int human_thread_id);
        
        virtual pid_t clarify_pid(name_pid * n_pid);
        virtual void output_serialised_results(void * args_ptr,
                                               void * serialise_ptr, 
                                               void * proc_mem_ptr);

};


#endif
