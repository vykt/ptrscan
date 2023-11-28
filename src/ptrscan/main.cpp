#include <iostream>
#include <stdexcept>

#include <linux/limits.h>

#include "ui_base.h"
#include "ui_term.h"
#include "args.h"
#include "proc_mem.h"

#ifdef DEBUG
#include "debug.h"
#endif

//DEBUG INCLUDES
#include <cstdio>


int main(int argc, char ** argv) {

    args_struct args;
    proc_mem p_mem; 

    ui_base * ui;

    //process cmdline arguments
    try {
        process_args(argc, argv, &args);
    } catch (const std::runtime_error & e) {
        std::cerr << e.what() << std::endl; //only handle ui exception this way
        return -1;
    }

    //instantiate ui
    if (args.ui_type == UI_TERM) {
        ui = new ui_term();
    } else {
        //TODO ncurses interface goes here, for now use term
        ui = new ui_term();
    }

    //instantiate memory object
    try {
        p_mem.init_proc_mem(&args, ui);
    } catch (const std::runtime_error& e) {
        ui->report_exception(e);
        return -1;
    }

    //TODO DEBUG: dump internal state
    #ifdef DEBUG
    dump_structures(&args, &p_mem);
    #endif

    std::cout << "press enter to terminate." << std::endl;
    getchar();

}
