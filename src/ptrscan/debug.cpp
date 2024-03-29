#include <iostream>

#include <cstdint>

#include <linux/limits.h>

#include "args.h"
#include "proc_mem.h"
#include "thread_ctrl.h"
#include "thread.h"
#include "mem_tree.h"

//for init dump
#define ARGS_MEMBERS 7
#define STATIC_REGION_MEMBERS 3
#define PROC_MEM_MEMBERS 5   //dont look at maps_data
#define MAPS_ENTRY_MEMBERS 4 //ignore caves member

//for gen tree dump
#define THREAD_CTRL_MEMBERS 3
#define THREAD_MEMBERS 2       //ignore level_barrier member
#define PARENT_RANGE_MEMBERS 3
#define MEM_RANGE_MEMBERS 3
#define MEM_TREE_MEMBERS 2     //ignore write_mutex member
#define MEM_NODE_MEMBERS 6


/*
 *  print to stderr & don't go through UI class (ncurses for debugging? :p)
 */

/*
 *  the odd spacing in the std::cerr's is to align the values
 */

/*
 *  as structure dumps grew in size this file become a cluster****, at some 
 *  point in the future it might be worthwhile to write a few generic member dump
 *  functions and use them instead of specifying the formatting by hand each time
 */

//dump args & libpwu memory structures
void dump_structures_init(args_struct * args, proc_mem * p_mem) {

    //args_structure members
    const char * a_mbr[ARGS_MEMBERS] = {
        "target_str",         //std::string
        "ui_type",            //byte
        "aligned",            //bool
        "ptr_lookback",       //uintptr_t
        "levels",             //unsigned int
        "num_threads",        //unsigned int
        "extra_static_vector" //std::vector<region>
    };

    //region members
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


    //start dump
    std::cerr << "\n*** *** *** *** *** [INIT DUMP] *** *** *** *** ***\n";

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
    for (unsigned int i = 0; i < args->extra_static_vector.size(); ++i) {
        std::cerr << '\n' << '\t' 
                  << "[DEBUG] --- (" << a_mbr[6] << ")[" << std::dec << i << "]:\n";
        std::cerr << '\t' << sr_mbr[0] << " : " 
                  << args->extra_static_vector[i].pathname << '\n'
                  << '\t' << sr_mbr[1] << "     : "
                  << args->extra_static_vector[i].skip << '\n'
                  << '\t' << sr_mbr[2] << "  : "
                  << args->extra_static_vector[i].skipped << '\n';
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
                  << '\t' << me_mbr[3] << "   : 0x"
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


//dump division of work between threads
void dump_structures_thread_work(thread_ctrl * t_ctrl) {

    //mem_range members
    const char * mr_mbr[MEM_RANGE_MEMBERS] = {
        "m_entry    : 0x", //maps_entry *
        "start_addr : 0x", //uintptr_t
        "end_addr   : 0x"  //uintptr_t
    };

    //thread members
    const char * t_mbr[THREAD_MEMBERS] = {
        "id: ",           //pthread_t (some kind of integer on Linux)
        "regions_to_scan" //std::vector<mem_range>
    };

    //start dump
    std::cerr << "\n*** *** *** *** *** [THREAD WORK DUMP] *** *** *** *** ***\n";

    //dump thread_ctrl.thread_vector
    for (unsigned int i = 0; i < (unsigned int) t_ctrl->thread_vector.size(); ++i) {
        std::cerr << '\n'
                  << "[DEBUG] --- (thread_vector)[" << i << "]:";
        std::cerr << t_mbr[0]
                  << t_ctrl->thread_vector[i].human_thread_id << '\n';

        //dump thread_ctrl.thread_vector[i].regions_to_scan
        for (unsigned int j = 0; 
             j < (unsigned int) t_ctrl->thread_vector[i].regions_to_scan.size(); ++j) {

            std::cerr << '\n' << '\t'
                      << "[DEBUG] --- (" << t_mbr[1] << ")[" << j << "]:";
            std::cerr << '\t' << mr_mbr[0]
                      << std::hex
                      << t_ctrl->thread_vector[i].regions_to_scan[j].m_entry << '\n'
                      << '\t' << mr_mbr[1]
                      << std::hex
                      << t_ctrl->thread_vector[i].regions_to_scan[j].start_addr << '\n'
                      << '\t' << mr_mbr[2]
                      << std::hex
                      << t_ctrl->thread_vector[i].regions_to_scan[j].end_addr << '\n';
        } //end inner for
    } //end for

}


//dump thread control level
void dump_structures_thread_level(thread_ctrl * t_ctrl, mem_tree * m_tree, int lvl) {

    //thread_ctrl object members
    const char * tc_mbr[THREAD_CTRL_MEMBERS] = {
        "current_level: ",        //unsigned int
        "parent_range_vector",    //std::vector<parent_range>
        "thread_vector"           //std::vector<thread>
    };

    //parent_range members
    const char * pr_mbr[PARENT_RANGE_MEMBERS] = {
        "parent_node (id) : ",   //mem_node * (unsigned int)
        "start_addr       : 0x", //uintptr_t
        "end_addr         : 0x"  //uintptr_t
    };

    //mem_tree members
    const char * mt_mbr[MEM_TREE_MEMBERS] = {
        "root_node (id): ",  //const mem_node * (unsigned int)
        "levels"             //std::vector<std::list<mem_node *>> * levels
    };

    //args_structure members
    const char * mn_mbr[MEM_NODE_MEMBERS] = {
        "id                   : ",   //unsigned int
        "static_regions_index : ",   //int
        "node_addr            : 0x", //node_addr
        "point_addr           : 0x", //point_addr
        "parent_node (id)     : ",   //mem_node * (unsigned int)
        "subnode_list (size)  : "    //std::list<mem_node> (int)
    };

    //pointer to m_tree->levels[i] for easier iteration
    std::list<mem_node *> * level_list;


    //start dump
    std::cerr << "\n*** *** *** *** *** [THREAD LEVEL DUMP: "
              << lvl 
              << "] *** *** *** *** ***\n";

    //dump thread_ctrl object (and its constituent threads)
    std::cerr << "\n[DEBUG] --- (thread_ctrl t_ctrl) --- CONTENTS:\n";
    std::cerr << tc_mbr[0] << t_ctrl->current_level << '\n';

    //dump thread_ctrl.parent_range_vector
    for (unsigned int i = 0; 
         i < (unsigned int) t_ctrl->parent_range_vector.size(); ++i) {
        
        std::cerr << '\n' << '\t'
                  << "[DEBUG] --- (" << tc_mbr[1] << ")[" << i << "]:" << '\n';
        std::cerr << '\t' << pr_mbr[0]
                  << std::dec << t_ctrl->parent_range_vector[i].parent_node->id << '\n'
                  << '\t' << pr_mbr[1]
                  << std::hex << t_ctrl->parent_range_vector[i].start_addr << '\n'
                  << '\t' << pr_mbr[2]
                  << std::hex << t_ctrl->parent_range_vector[i].end_addr << '\n';
    } //end for


    //dump memory tree
    std::cerr << "\n[DEBUG] --- (mem_tree m_tree) --- CONTENTS:\n";
    std::cerr << mt_mbr[0] << m_tree->root_node->id << '\n';

    //dump memory nodes for level
    level_list = &(*m_tree->levels)[lvl];
    for (std::list<mem_node *>::iterator it = level_list->begin();
         it != level_list->end(); ++it) {

        std::cerr << '\n' << '\t'
                  << "[DEBUG] --- (" << mt_mbr[1] << ")[" << lvl << "]:" << '\n';
        std::cerr << '\t' << mn_mbr[0]
                  << std::dec << (*it)->id << '\n'
                  << '\t' << mn_mbr[1]
                  << std::dec << (*it)->static_regions_index << '\n'
                  << '\t' << mn_mbr[2]
                  << std::hex << (*it)->node_addr << '\n'
                  << '\t' << mn_mbr[3]
                  << std::hex << (*it)->point_addr << '\n'
                  << '\t' << mn_mbr[4]
                  << std::dec << (*it)->parent_node->id << '\n'
                  << '\t' << mn_mbr[5]
                  << std::dec << (*it)->subnode_list.size() << '\n';
    } //end for

    //dump 

}
