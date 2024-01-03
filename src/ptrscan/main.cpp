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
#include "serialise.h"
#include "verify.h"

#ifdef DEBUG
#include "debug.h"
#endif

//DEBUG INCLUDES
#include <cstdio>



//generate pointer tree
void threaded_scan(args_struct * args, proc_mem * p_mem, 
                   mem_tree ** m_tree, ui_base * ui) {
  
    //TODO USE UI TO REPORT PROGRESS OF SCAN

    //instantiate pointer map tree
    thread_ctrl * t_ctrl;
    (*m_tree) = new mem_tree(args, p_mem);

    //initialise the thread controller
    t_ctrl = new thread_ctrl();
    t_ctrl->init(args, p_mem, *m_tree, p_mem->pid);

    #ifdef DEBUG
    dump_structures_thread_work(t_ctrl);
    #endif

    //for every level
    for (unsigned int i = 1; i < args->levels; ++i) {
        
        //prepare the next level
        t_ctrl->prepare_level(args, p_mem, *m_tree);

        //start next level
        t_ctrl->start_level();

        //end level
        t_ctrl->end_level();

        #ifdef DEBUG
        dump_structures_thread_level(t_ctrl, *m_tree, i);
        #endif

    } //end for every level

    //wait on threads to terminate
    t_ctrl->wait_thread_terminate();
}


//main
int main(int argc, char ** argv) {

    int mode;

    //allocated here
    args_struct args;
    proc_mem p_mem;
    serialise ser;

    ui_base * ui;

    //allocated in called functions
    mem_tree * m_tree;



    //STAGE I - INPUT

    //process cmdline arguments
    try {
        mode = process_args(argc, argv, &args);
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

    #ifdef DEBUG
    dump_structures_init(&args, &p_mem);
    #endif

    //STAGE II - PROCESS MODE

    /*
     *  This switch statement can be made a lot more concise at the cost of clarity.
     */

    //switch based on return from process_args()
    switch (mode) {

        //scan and output results, do not save to a file
        case MODE_SCAN:
            
            try {
                //carry out scan
                threaded_scan(&args, &p_mem, &m_tree, ui); 
                
                //serialise results
                ser.tree_to_results(&args, &p_mem, m_tree);

                //output results
                ui->output_serialised_results(&args, &ser, &p_mem);
            } catch (std::runtime_error& e) {
                ui->report_exception(e);
            }

            break;

        //scan, output results and save to a file
        case MODE_SCAN_WRITE:

            try {
                //carry out scan
                threaded_scan(&args, &p_mem, &m_tree, ui); 
                
                //serialise results
                ser.tree_to_results(&args, &p_mem, m_tree);

                //write results to disk
                ser.write_mem_ptrchains(&args, &p_mem);

                //output results
                ui->output_serialised_results(&args, &ser, &p_mem);
            } catch (std::runtime_error& e) {
                ui->report_exception(e);
            }
            
            break;

        //do not scan, only print results read from a file
        case MODE_READ:

            try {
                //read results from disk
                ser.read_disk_ptrchains(&args);

                //output results
                ui->output_serialised_results(&args, &ser, &p_mem);
            } catch (std::runtime_error& e) {
                ui->report_exception(e);
            }

            break;

        //rescan based on file, print results, do not save to file
        case MODE_RESCAN:

            try {
                //read results from disk
                ser.read_disk_ptrchains(&args);

                //verify results
                verify(&args, &p_mem, ui, &ser);

                //output results
                ui->output_serialised_results(&args, &ser, &p_mem);
            } catch (std::runtime_error& e) {
                ui->report_exception(e);
            }


            break;

        //rescan based on file, print results, save to a file
        case MODE_RESCAN_WRITE:

            try {
                //read results from disk
                ser.read_disk_ptrchains(&args);

                //verify results
                verify(&args, &p_mem, ui, &ser);

                //write results to disk
                ser.write_mem_ptrchains(&args, &p_mem);

                //output results
                ui->output_serialised_results(&args, &ser, &p_mem);
            } catch (std::runtime_error& e) {
                ui->report_exception(e);
            }

            break;

    } //end switch

    //std::cout << "press enter to terminate." << std::endl;
    //getchar();

}
