#include <iostream>
#include <string>
#include <sstream>

#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include <linux/limits.h>

#include <pthread.h>
#include <libcmore.h>
#include <liblain.h>

#include "ui_term.h"
#include "args.h"
#include "mem.h"
#include "serialiser.h"



//get longest basename
static inline void _get_column_sizes(const args_struct * args, const serialiser * s, 
                                     std::vector<int> * column_sizes) {

    int max;
    int column_index;
    int offset_len;
    int current_basename_len;
    uint32_t current_offset;

    max = -1;
    column_index = 0;

    serial_entry * s_entry;
    cm_list_node * obj_node;
    ln_vm_obj * obj;

    const std::vector<serial_entry> * ptrchains   = s->get_ptrchains();
    const std::vector<cm_list_node *> * rw_objs   = s->get_rw_objs();
    const std::vector<std::string> * read_rw_objs = s->get_read_rw_objs();

    //for every pointer chain
    for (int i = 0; i < (int) ptrchains->size(); ++i) {

        s_entry = (serial_entry *) &(*ptrchains)[i];

        //if in read mode
        if (args->mode == MODE_READ) {
            current_basename_len = strnlen((*read_rw_objs)
                                           [s_entry->rw_objs_index].c_str(), NAME_MAX);

        //else in scan / verify modes
        } else {
        
            //get object
            obj_node = (*rw_objs)[s_entry->rw_objs_index];
            obj = LN_GET_NODE_OBJ(obj_node);

            //get length of basename
            current_basename_len = strnlen(obj->basename, NAME_MAX);

        } //end if

        //update max as required
        if (max < current_basename_len) max = current_basename_len;
    }

    //push first column size
    column_sizes->push_back(max);


    //get width of all remaining offset columns
    do {

        max = -1;

        //for every serial entry
        for (int i = 0; i < (int) ptrchains->size(); ++i) {

            //get offset of next serial entry at given column
            if ((int) (*ptrchains)[i].offsets.size() <= column_index) continue;
            
            current_offset = (*ptrchains)[i].offsets[column_index];
            
            //get length through division
            offset_len = 0;
            while (current_offset != 0) {
                current_offset /= 0x10;
                offset_len += 1;
            }

            //update max as required
            if (max < offset_len) max = offset_len;

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
inline void ui_term::report_depth_progress(const int depth_done) {
    
    std::cout << RED << "[ctrl]: level " << depth_done << " finished.\n" << RESET;
    return;
}


//acquire mutex & report thread progress
inline void ui_term::report_thread_progress(const unsigned int vma_done, 
                                            const unsigned int vma_total,
                                            const int human_thread_id) {

    //to avoid using mutexes here, compose the string & call std::cout once
    std::stringstream report;
    report << "[thread " << human_thread_id << "] " << vma_done << " out of "
           << vma_total << " finished.\n";
    std::cout << report.str();

    return;
}


/*
 *  This is not efficient. It is also just printing the results.
 */

//print out results
void ui_term::output_ptrchains(const void * args_ptr, const void * s_ptr) {

    int len;
    char * basename;

    serial_entry * s_entry;
    cm_list_node * obj_node;
    ln_vm_obj * obj;

    std::string print_buf;
    std::stringstream convert_buf;

    std::vector<int> column_sizes;

    //typecast arg
    args_struct * args = (args_struct *) args_ptr;
    serialiser * s = (serialiser *) s_ptr;

    //get serialiser vectors
    const std::vector<serial_entry> * ptrchains   = s->get_ptrchains();
    const std::vector<cm_list_node *> * rw_objs   = s->get_rw_objs();
    const std::vector<std::string> * read_rw_objs = s->get_read_rw_objs();

    //get column widths
    _get_column_sizes(args, s, &column_sizes);

    
    //output header if not reading
    if (args->mode != MODE_READ) {
        std::cout << "\ntarget name: " << args->target_str 
                  << " | target addr: 0x" << std::hex << args->target_addr
                  << std::dec << "\n\n";
    } else {
        std::cout << "\nreading file: " << args->input_file << "\n\n";
    }

    //for every pointer chain
    for (unsigned int i = 0; i < ptrchains->size(); ++i) {

        //get pointer for next serial_entry
        s_entry = (serial_entry *) &(*ptrchains)[i];

        //setup buffer
        print_buf.clear();

        //set color
        if (args->colour && ((int) s_entry->static_objs_index != -1)) {
            print_buf.append(GREEN);
        }

        //get basename of obj for pointer chain
        if (args->mode == MODE_READ) {
            basename = (char *) (*read_rw_objs)[s_entry->rw_objs_index].c_str();

        } else {
            obj_node = (*rw_objs)[s_entry->rw_objs_index];
            obj = LN_GET_NODE_OBJ(obj_node);
            basename = obj->basename; 
        }

        //format basename
        len = column_sizes[0] - strlen(basename);
        print_buf.append(len, ' ');
        print_buf.append(basename);
        print_buf.append(" + ");

        //format remaining offsets
        for (int j = 0; j < (int) s_entry->offsets.size(); ++j) {

            //format offsets
            convert_buf.str(std::string());
            convert_buf.clear();
            convert_buf << "0x" << std::hex << s_entry->offsets[j];
            print_buf.append(convert_buf.str());
            if ((column_sizes[j+1] - (convert_buf.str().length() - 2)) != 0) {
                print_buf.append(column_sizes[j+1]
                                 - (convert_buf.str().length() - 2), ' ');
            }
            print_buf.append(4, ' ');

        } //end inner for

        //unset color
        if (args->colour && ((int) s_entry->static_objs_index != -1)) {
            print_buf.append(RESET);
        }

        //write line
        print_buf.append("\n");
        std::cout << print_buf;

    } //end for
   
    //output footer
    std::cout << "\nfound " << ptrchains->size() 
              << " chains | byte width: "  << (int) s->get_byte_width()
              << " | alignment: " << (int) s->get_alignment()
              << "\nmax struct size: 0x " << std::hex << s->get_max_struct_size()
              << " | max depth: " << std::dec << s->get_max_depth()
              << std::endl;
}
