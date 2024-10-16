#include <iostream>
#include <stdexcept>

#include <linux/limits.h>

#include "ui_base.h"
#include "ui_term.h"
#include "args.h"
#include "mem.h"
#include "thread_ctrl.h"
#include "mem_tree.h"
#include "serialiser.h"
#include "debug.h"



//generate pointer tree
static inline void _do_scan(const args_struct * args, mem * m, 
                                  mem_tree * m_tree, ui_base * ui) {

    //instantiate pointer map tree
    thread_ctrl * t_ctrl;

    //initialise the thread controller
    t_ctrl = new thread_ctrl(args, m, m_tree, ui);

    #ifdef DEBUG
    dump_threads(t_ctrl);
    #endif

    //for every level
    for (int i = 1; i < args->max_depth; ++i) {
        
        //prepare the next level
        t_ctrl->prepare_threads(args, m, m_tree);

        //start next level
        t_ctrl->start_level();

        //end level
        t_ctrl->end_level();

        //report end of level
        if (args->verbose) {
            ui->report_depth_progress(i);
        }

    } //end for every level

    //wait on threads to terminate
    t_ctrl->join_threads();

    #ifdef DEBUG
    dump_mem_tree(m_tree);
    #endif

    return;
}


static inline void _mode_scan(args_struct * args, mem * m,
                              mem_tree * m_tree, serialiser * s, ui_base * ui) {

    //populate rw-/rwx & static areas
    m->populate_areas(args);

    //carry out scan
    _do_scan(args, m, m_tree, ui); 
    
    //serialise results
    s->serialise_tree(args, m, m_tree);

    #ifdef DEBUG
    dump_serialiser(s);
    #endif

    //output results
    ui->output_ptrchains(args, s, m);

    return;
}


static inline void _mode_scan_write(args_struct * args, mem * m,
                                    mem_tree * m_tree, serialiser * s, ui_base * ui) {

    //populate rw-/rwx & static areas
    m->populate_areas(args);

    //carry out scan
    _do_scan(args, m, m_tree, ui); 
    
    //serialise results
    s->serialise_tree(args, m, m_tree);

    //save results
    s->record_pscan(args, m);

    #ifdef DEBUG
    dump_serialiser(s);
    #endif 

    //output results
    ui->output_ptrchains(args, s, m);

    return;
}


static inline void _mode_read(args_struct * args, mem * m,
                              mem_tree * m_tree, serialiser * s, ui_base * ui) {

    //read results
    s->read_pscan(args, m);

    //output results
    ui->output_ptrchains(args, s, m);

    return;
}


static inline void _mode_verify(args_struct * args, mem * m,
                                mem_tree * m_tree, serialiser * s, ui_base * ui) {

    //read results
    s->read_pscan(args, m);

    //verify results
    s->verify(args, m);

    #ifdef DEBUG
    dump_serialiser(s);
    #endif

    //output results
    ui->output_ptrchains(args, s, m);

    return;
}



static inline void _mode_verify_write(args_struct * args, mem * m,
                                      mem_tree * m_tree, serialiser * s, 
                                      ui_base * ui) {

    //read results
    s->read_pscan(args, m);

    //verify results
    s->verify(args, m);

    //save results
    s->record_pscan(args, m);

    #ifdef DEBUG
    dump_serialiser(s);
    #endif

    //output results
    ui->output_ptrchains(args, s, m);

    return;
}


int main(int argc, char ** argv) {

    int mode;

    //options
    args_struct args;
    
    //objects
    mem * m;
    mem_tree * m_tree;
    serialiser * s;
    ui_base * ui;


    // --- STAGE 1: setup

    try {
        //process cmdline arguments
        mode = process_args(argc, argv, &args);

        //instantiate ui
        if (args.ui_type == UI_TERM) {
            ui = new ui_term();
        } else {
            //TODO ncurses interface goes here, for now use term
            ui = new ui_term();
        }

        //instantiate memory manager
        m = new mem(&args);

        //instantiate memory tree
        m_tree = new mem_tree(&args, m);

        //instantiate a serialiser
        s = new serialiser();

    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl; //only handle ui exception this way
        return -1;
    }


    #ifdef DEBUG
    dump_args(&args);
    dump_mem(m);
    #endif


    // --- STAGE 2: processing
    try {

        //switch based on return from process_args()
        switch (mode) {

            //scan and output results, do not save to a file
            case MODE_SCAN:

                _mode_scan(&args, m, m_tree, s, ui);
                break;


            //scan, output results and save to a file
            case MODE_SCAN_WRITE:

                _mode_scan_write(&args, m, m_tree, s, ui);
                break;

            //do not scan, only print results read from a file
            case MODE_READ:

                _mode_read(&args, m, m_tree, s, ui);
                break;

            //rescan based on file, print results, do not save to file
            case MODE_VERIFY:

                _mode_verify(&args, m, m_tree, s, ui);
                break;

            //rescan based on file, print results, save to a file
            case MODE_VERIFY_WRITE:

                _mode_verify_write(&args, m, m_tree, s, ui);
                break;

        } //end switch
    
    } catch (std::runtime_error & e) {
        ui->report_exception(e);
    }

    return 0;
}
