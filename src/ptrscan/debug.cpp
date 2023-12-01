#include <iostream>

#include <cstdint>

#include <linux/limits.h>

#include "args.h"
#include "proc_mem.h"

#define ARGS_MEMBERS 7
#define STATIC_REGION_MEMBERS 3
#define PROC_MEM_MEMBERS 5   //dont look at maps_data
#define MAPS_ENTRY_MEMBERS 4 //ignore caves member


/*
 *  print to stderr & don't go through UI class (ncurses for debugging? :p)
 */

/*
 *  the odd spacing in the std::cerr's is to align the values
 */

//dump non-tree structures
void dump_structures(args_struct * args, proc_mem * p_mem) {

    //args_structure members
    const char * a_mbr[ARGS_MEMBERS] = {
        "target_str",         //std::string
        "ui_type",            //byte
        "aligned",            //bool
        "ptr_lookback",       //uintptr_t
        "levels",             //unsigned int
        "num_threads",        //unsigned int
        "extra_region_vector" //std::vector<static_region>
    };

    //static_region members
    const char * sr_mbr[STATIC_REGION_MEMBERS] {
        "pathname", //std::string
        "skip",     //int
        "skipped"   //int
    };

    const char * pm_mbr[PROC_MEM_MEMBERS] {
        "pid",                  //pid_t
        "mem_fd",               //int
        "maps_stream",          //FILE *
        "rw_regions_vector",    //std::vector<maps_entry *>
        "static_regions_vector" //std::vector<maps_entry *>
    };

    //maps_entry members, pulled from man 3 libpwu_structs
    const char * me_mbr[MAPS_ENTRY_MEMBERS] {
        "pathname",   //char[PATH_MAX]
        "perms",      //byte
        "start_addr", //void * (uintptr_t)
        "end_addr"    //void * (uintptr_t)
    };


    //dump args structure
    std::cerr << "[DEBUG] --- (args_struct args) --- CONTENTS:\n";
    std::cerr << a_mbr[0] << "   : " << args->target_str << '\n'
              << a_mbr[1] << "      : " << (unsigned int) args->ui_type << '\n'
              << a_mbr[2] << "      : " << args->aligned << '\n'
              << a_mbr[3] << " : 0x" << std::hex << args->ptr_lookback << '\n'
              << a_mbr[4] << "       : " << std::dec << args->levels << '\n'
              << a_mbr[5] << "       : " << args->num_threads << '\n'
              << " --- " << a_mbr[6] << ": \n";
    
    //dump args.extra_static_vector
    for (unsigned int i = 0; i < args->extra_region_vector.size(); ++i) {
        std::cerr << '\n' << '\t' 
                  << "[DEBUG] --- (" << a_mbr[6] << ")[" << i << "]:\n";
        std::cerr << '\t' << sr_mbr[0] << " : " 
                  << args->extra_region_vector[i].pathname << '\n'
                  << '\t' << sr_mbr[1] << "     : "
                  << args->extra_region_vector[i].skip << '\n'
                  << '\t' << sr_mbr[2] << "  : "
                  << args->extra_region_vector[i].skipped << '\n';
    } //end for
    
    //dump p_mem part 1
    std::cerr << '\n' << "[DEBUG] --- (proc_mem p_mem) --- CONTENTS:\n";
    std::cerr << pm_mbr[0] << "         : " <<  std::dec << p_mem->pid << '\n'
              << pm_mbr[1] << "      : " << p_mem->mem_fd << '\n'
              << pm_mbr[2] << " : 0x" 
                           << std::hex << (uintptr_t) p_mem->maps_stream << '\n'
              << " --- " << pm_mbr[3] << ": \n";

    //dump std::vector<maps_entry *> p_mem.rw_regions_vector;
    for (unsigned int i = 0; i < p_mem->rw_regions_vector.size(); ++i) {

        std::cerr << '\n' << '\t'
                  << "[DEBUG] --- (" << pm_mbr[3] << ")[" << i << "]:\n";
        std::cerr << '\t' << me_mbr[0] << "   : " 
                  << p_mem->rw_regions_vector[i]->pathname << '\n'
                  << '\t' << me_mbr[1] << "      : "
                  << (unsigned int) p_mem->rw_regions_vector[i]->perms << '\n'
                  << '\t' << me_mbr[2] << " : 0x"
                  << (uintptr_t) p_mem->rw_regions_vector[i]->start_addr << '\n'
                  << '\t' << me_mbr[3] << " : 0x"
                  << (uintptr_t) p_mem->rw_regions_vector[i]->end_addr << '\n';
    } //end for

    //dumo p_mem part 2
    std::cerr << '\n' << " --- " << pm_mbr[4] << ": \n";

    //dump std::vector<maps_entry *> p_mem.static_regions_vector;
    for (unsigned int i = 0; i < p_mem->static_regions_vector.size(); ++i) {
    
        std::cerr << '\n' << '\t'
                  << "[DEBUG] --- (" << pm_mbr[4] << ")[" << i << "]:\n";
        std::cerr << '\t' << me_mbr[0] << "   : " 
                  << p_mem->static_regions_vector[i]->pathname << '\n'
                  << '\t' << me_mbr[1] << "      : "
                  << (unsigned int) p_mem->static_regions_vector[i]->perms << '\n'
                  << '\t' << me_mbr[2] << " : 0x"
                  << (uintptr_t) p_mem->static_regions_vector[i]->start_addr << '\n'
                  << '\t' << me_mbr[3] << "   : 0x"
                  << (uintptr_t) p_mem->static_regions_vector[i]->end_addr << '\n';

    } //end for

    


}
