#ifndef UI_TERM_H
#define UI_TERM_H

#include <vector>
#include <string>

#include <libpwu.h>

#include "ui_base.h"


//virtual ui class, inherited from by terminal and tui ncurses interfaces
class ui_term : public ui_base {

    public:
        virtual int process_args(int argc, char ** argv);
        virtual int clarify_pid(name_pid * n_pid);

};


#endif
