#include <iostream>
#include <stdexcept>

#include <linux/limits.h>

#include "ui_base.h"
#include "ui_term.h"
#include "args.h"
#include "proc_mem.h"
#include "thread_ctrl.h"
#include "thread.h"
#include "mem_tree.h"

#ifdef DEBUG
#include "debug.h"
#endif

//DEBUG INCLUDES
#include <cstdio>



//generate pointer tree
void threaded_scan(args_struct * args, proc_mem * p_mem, thread_ctrl * t_ctrl,
                   mem_tree * m_tree, ui_base * ui, pid_t pid) {
  
    //TODO USE UI TO REPORT PROGRESS OF SCAN

    //initialise the thread controller
    t_ctrl->init(args, p_mem, m_tree, pid);

    #ifdef DEBUG
    dump_structures_thread_work(t_ctrl);
    #endif

    //for every level
    for (unsigned int i = 1; i < args->levels; ++i) {
        
        //prepare the next level
        t_ctrl->prepare_level(args, p_mem, m_tree);

        //start next level
        t_ctrl->start_level();

        //end level
        t_ctrl->end_level();

        #ifdef DEBUG
        dump_structures_thread_level(t_ctrl, m_tree, i);
        #endif

    } //end for every level

    //wait on threads to terminate
    t_ctrl->wait_thread_terminate();
}


//main
int main(int argc, char ** argv) {

    args_struct args;
    proc_mem p_mem; 
    thread_ctrl t_ctrl;

    ui_base * ui;
    mem_tree * m_tree;


    //STAGE I - INPUT

    //process cmdline arguments
    try {
        process_args(argc, argv, &args);
    } catch (const std::runtime_error& e) {
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

    //TODO DEBUG: dump internal args & libpwy memory structures state
    #ifdef DEBUG
    dump_structures_init(&args, &p_mem);
    #endif

    //STAGE II - TREE GEN

    //instantiate pointer map tree
    try {
        m_tree = new mem_tree(&args, &p_mem);
    } catch (std::runtime_error& e) {
        ui->report_exception(e);
        return -1;
    }

    //scan tree
    try {
        threaded_scan(&args, &p_mem, &t_ctrl, m_tree, ui, p_mem.pid);
    } catch (std::runtime_error& e) {
        ui->report_exception(e);
        return -1;
    }

    std::cout << "press enter to terminate." << std::endl;
    getchar();

}
