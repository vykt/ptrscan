#ifndef UI_TERM_H
#define UI_TERM_H

#include <vector>
#include <string>

#include <cstdint>

#include <libpwu.h>

#include "ui_base.h"


//virtual ui class, inherited from by terminal and tui ncurses interfaces
class ui_term : public ui_base {

    public:
        virtual void report_exception(const std::exception& e);
        virtual pid_t clarify_pid(name_pid * n_pid);

};


#endif
