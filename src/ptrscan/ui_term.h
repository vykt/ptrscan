#ifndef UI_TERM_H
#define UI_TERM_H

#include <vector>
#include <string>

#include <cstdint>

#include <libpwu.h>

#include "ui_base.h"


#define RESET "\033[0m"
#define RED   "\033[31m"
#define GREEN "\033[32m"


//virtual ui class, inherited from by terminal and tui ncurses interfaces
class ui_term : public ui_base {

    public:
        virtual void report_exception(const std::exception& e);
        virtual pid_t clarify_pid(name_pid * n_pid);
        virtual void output_serialised_results(void * serialise_ptr, 
                                               void * proc_mem_ptr);

};


#endif
