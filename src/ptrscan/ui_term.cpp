#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include <pthread.h>
#include <libpwu.h>

#include "ui_term.h"
#include "ui_base.h"
#include "args.h"
#include "serialise.h"
#include "proc_mem.h"



//get maps_obj by name
inline void fill_serial_to_obj(serialise * ser, proc_mem * p_mem) {

    const char * exception_str[2] = {
        "serial_to_obj: static or rw indeces failed to produce an object",
        "serial_to_obj: static and rw indeces unset."
    };


    int ret;
    serial_entry * temp_s_entry;

    
    //for every pointer chain
    for (unsigned int i = 0; i < ser->ptrchains_vector.size(); ++i) {

        //get pointer for next serial_entry
        temp_s_entry = &ser->ptrchains_vector[i];

        /*
         *  Priority given to matching by static region because it is more reliable
         */

        //match region using static_region_index
        if (temp_s_entry->static_regions_index != -1) {

            //get parent object of matching static region
            ret = match_maps_obj(
                    ser->static_region_str_vector[
                    temp_s_entry->static_regions_index],
                    p_mem,
                    &temp_s_entry->matched_m_obj);
            if (ret == -1) {
                throw std::runtime_error(exception_str[0]);
            }

        //otherwise fallback to using rw_region_index 
        } else if (temp_s_entry->rw_regions_index != -1) {

            //unset color
            std::cout << RESET;
            
            //get parent object of matching static region
            ret = match_maps_obj(
                    ser->rw_region_str_vector[
                    temp_s_entry->rw_regions_index],
                    p_mem,
                    &temp_s_entry->matched_m_obj);
            if (ret == -1) {
                throw std::runtime_error(exception_str[0]);
            }

        //this should never happen
        } else {
            throw std::runtime_error(exception_str[1]);
        
        } //end if-else-then

    } //end for every ptrchain
}


//get longest basename
inline void get_column_sizes(serialise * ser, proc_mem * p_mem, 
                            std::vector<int> * column_sizes) {

    int max, now, len, column_index;
    uint32_t unow;

    max = -1;
    column_index = 0;


    //first, get basename width
    for (unsigned int i = 0; i < (unsigned int) ser->ptrchains_vector.size(); ++i) {

        //get length of basename
        now = strlen(ser->ptrchains_vector[i].matched_m_obj->basename);

        //update max as required
        if (max < now) max = now;
    }

    column_sizes->push_back(max);


    //get width of all remaining offset columns
    do {

        max = -1;

        //for every serial entry
        for (unsigned int i = 0; i < (unsigned int) ser->ptrchains_vector.size(); ++i) {

            //get offset of next serial entry at given column
            if (ser->ptrchains_vector[i].offset_vector->size()
                <= (unsigned int) column_index) continue;
            
            unow = (*ser->ptrchains_vector[i].offset_vector)[column_index];
            
            //get length through division
            len = 0;
            while (unow != 0) {
                unow /= 0x10;
                len += 1;
            }

            //update max as required
            if (max < len) max = len;

        } //end for

        column_index += 1;
        if (max != -1) column_sizes->push_back(max);

    } while (max != -1);

    return;
}


//report exception to user
inline void ui_term::report_exception(const std::exception& e) {

    std::cerr << e.what() << std::endl;
    return;
}


//report when a level is finished
inline void ui_term::report_control_progress(int level_done) {
    
    std::cout << RED << "[ctrl]: level " << level_done << " finished.\n" << RESET;
    return;
}


//acquire mutex & report thread progress
inline void ui_term::report_thread_progress(unsigned int region_done, 
                                            unsigned int region_total,
                                            int human_thread_id) {

    //to avoid using mutexes here, compose the string & call std::cout once
    std::stringstream report;
    report << "[thread " << human_thread_id << "] " << region_done << " out of "
           << region_total << " finished.\n";
    std::cout << report.str();

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


    std::cout << "please type a PID matching the target name:" << std::endl;

    //for every match
    for (unsigned int i = 0; i < n_pid->pid_vector.length; ++i) {
        
        ret = vector_get_ref(&n_pid->pid_vector, (unsigned long) i,
                             (byte **) &temp_pid);
        if (ret == -1) {
            throw std::runtime_error(exception_str[0]);
        }

        std::cout << i << ": " << *temp_pid << std::endl;
    }

    //ask until correct input is provided (they'll just type the PID i know it)
    while (1) {

        input.clear();
        std::getline(std::cin, input);
        try {
            std::cout << "> ";
            selection = std::stoi(input);
						break;
        } catch (std::exception& e) {
            std::cout << std::endl << "invalid selection provided";
        }
    } //end while

    return selection;
}



/*
 *  This is not efficient. It is also just printing the results.
 */

//print out results
void ui_term::output_serialised_results(void * args_ptr, void * serialise_ptr,
                                        void * proc_mem_ptr) {

    int len;

    serial_entry * temp_s_entry;

    std::string print_buf;
    std::stringstream convert_buf;

    std::vector<int> column_sizes;

    //typecast arg
    args_struct * real_args = (args_struct *) args_ptr;
    serialise * real_serialise = (serialise *) serialise_ptr;
    proc_mem * real_p_mem = (proc_mem *) proc_mem_ptr;

    //find corresponding maps_obj for each serial entry
    fill_serial_to_obj(real_serialise, real_p_mem);

    //get column widths
    get_column_sizes(real_serialise, real_p_mem, &column_sizes);


    //output header
    std::cout << "\nptrscan for: " << real_args->target_str 
              << " | target addr: 0x" << std::hex << real_args->target_addr
              << "\n\n";

    //for every pointer chain
    for (unsigned int i = 0; i < real_serialise->ptrchains_vector.size(); ++i) {

        //get pointer for next serial_entry
        temp_s_entry = &real_serialise->ptrchains_vector[i];

        //setup buffer
        print_buf.clear();

        //set color
        if (temp_s_entry->static_regions_index != -1) {
            print_buf.append(GREEN);
        }

        //format basename
        len = column_sizes[0] - strlen(temp_s_entry->matched_m_obj->basename);
        print_buf.append(len, ' ');
        print_buf.append(temp_s_entry->matched_m_obj->basename);
        print_buf.append(" + ");

        //format remaining offsets
        for (unsigned int j = 0; j < temp_s_entry->offset_vector->size(); ++j) {

            //format offsets
            convert_buf.str(std::string());
            convert_buf.clear();
            convert_buf << "0x" << std::hex << (*temp_s_entry->offset_vector)[j];
            print_buf.append(convert_buf.str());
            if ((column_sizes[j+1] - (convert_buf.str().length() - 2)) != 0) {
                print_buf.append(column_sizes[j+1]
                                 - (convert_buf.str().length() - 2), ' ');
            }
            print_buf.append(4, ' ');

        } //end inner for

        //unset color
        if (temp_s_entry->static_regions_index != -1) {
            print_buf.append(RESET);
        }

        //write line
        print_buf.append("\n");
        std::cout << print_buf;

    } //end for
   
    //output footer
    std::cout << "\nfound " << std::dec << real_serialise->ptrchains_vector.size() 
              << " chains | aligned: "
              //this is 'magic' to convert bool into alignment of scan in bytes
              << (((int) real_args->aligned) * (sizeof(uintptr_t) - 1) + 1)
              << " | lookback: 0x" << std::hex << real_args->ptr_lookback 
              << " | levels: " << std::dec << real_args->levels 
              << std::endl;
}
