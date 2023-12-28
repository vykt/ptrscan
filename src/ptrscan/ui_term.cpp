#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#include <unistd.h>

#include <libpwu.h>

#include "ui_term.h"
#include "ui_base.h"
#include "serialise.h"
#include "proc_mem.h"



//report exception to user
inline void ui_term::report_exception(const std::exception& e) {

    std::cerr << e.what() << std::endl;
    return;
}


//get user to pick one PID from multiple matches
pid_t ui_term::clarify_pid(name_pid * n_pid) {

    const char * exception_str[1] = {
        "ui_term -> clarify_pid: vector range error."
    };

    int ret;
    std::string input;
    pid_t selection;
    pid_t * temp_pid;
    std::string buf;


    std::cout << "please select a PID matching the target name:" << std::endl;

    //for every match
    for (unsigned int i = 0; i < n_pid->pid_vector.length; ++i) {
        
        ret = vector_get_ref(&n_pid->pid_vector, (unsigned long) i,
                             (byte **) &temp_pid);
        if (ret == -1) {
            throw std::runtime_error(exception_str[0]);
        }

        std::cout << i << ": " << temp_pid << std::endl;
    }

    //ask until correct input is provided (they'll just type the PID i know it)
    while (1) {

        input.clear();
        std::getline(std::cin, input);
        try {
            selection = std::stoi(input);
        } catch (std::exception& e) {
            std::cout << "invalid selection provided";
        }
    } //end while

    //TODO then fix proc_mem::fetch_pid, it needs to call this
    return selection;
}



/*
 *  This function does a lot of searching, which can be deemed unnecessary if the 
 *  program is working on results it itself generated.
 *
 *  However since this function also gets called on results read from disk, and 
 *  program /proc/pid/maps may be different across different instances of execution,
 *  it becomes necessary to match results stored in the serialise class with what is 
 *  actually found in memory.
 */

//print out results
void ui_term::output_serialised_results(void * serialise_ptr, void * proc_mem_ptr) {

    const char * exception_str[2] = {
        "ui_term -> output_serialised_results: vector_get_ref() failed on maps_obj.",
        "ui_term -> output_serialised_results: static and rw indeces didn't match."
    };


    int ret;
    
    serial_entry * temp_s_entry;
    maps_obj * matched_m_obj;

    std::string print_buf;
    std::stringstream convert_buf;

    //typecast arg
    serialise * real_serialise = (serialise *) serialise_ptr;
    proc_mem * real_p_mem = (proc_mem *) proc_mem_ptr;


    //for every pointer chain
    for (unsigned int i = 0; i < real_serialise->ptrchains_vector.size(); ++i) {

        //get pointer for next serial_entry
        temp_s_entry = &real_serialise->ptrchains_vector[i];

        /*
         *  Priority given to matching by static region because it is more reliable
         */

        //match region using static_region_index
        if (temp_s_entry->static_regions_index != -1) {
            
            //TODO SET COLOR

            //get parent object of matching static region
            ret = match_maps_obj(
                    real_serialise->static_region_str_vector[
                    temp_s_entry->static_regions_index],
                    proc_mem_ptr,
                    &matched_m_obj);
            if (ret == -1) {
                //TODO say "could not find matching static entry in memory"
            }

        //otherwise fallback to using rw_region_index 
        } else if (temp_s_entry->rw_regions_index != -1) {

            //TODO UNSET COLOR
            
            //get parent object of matching static region
            ret = match_maps_obj(
                    real_serialise->rw_region_str_vector[
                    temp_s_entry->rw_regions_index],
                    proc_mem_ptr,
                    &matched_m_obj);
            if (ret == -1) {
                //TODO say "could not find matching static entry in memory"
            }

        //this should never happen
        } else {
            throw std::runtime_error(exception_str[1]);
        
        } //end if-else-then

        //setup buffers
        convert_buf.str(std::string());
        convert_buf.clear();
        print_buf.clear();

        //setup string
        convert_buf << std::hex << (*temp_s_entry->offset_vector)[0];
        print_buf.append(matched_m_obj->basename);
        print_buf.append("+0x");
        print_buf.append(convert_buf.str());

        //align offsets
        std::string offset_align(print_buf.length() % 4, ' ');

        //output basename + first offset
        std::cout << offset_align << print_buf << ' ';

        //iterate through the remaining offsets
        for (unsigned int j = 1; 
             i < (unsigned int) temp_s_entry->offset_vector->size(); ++j) {

            std::cout << "  0x" << std::hex << (*temp_s_entry->offset_vector)[j];

        } //end for
        std::cout << '\n';
        
    } //end for
    
}
