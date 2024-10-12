#ifndef SERIALISE_H
#define SERIALISE_H

#include <vector>

#include <cstdint>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "mem_tree.h"


#define DELIM_REGION 0
#define DELIM_OFFSET 0xFFFFFFF0

#define STATIC_REGIONS 0
#define RW_REGIONS 1
#define REGIONS_TYPE_NUM 2


//single set of offsets
typedef struct {

    //read & write use (set by tree_to_results OR read_disk_ptrchains)
    int rw_regions_index;
    int static_regions_index;
    std::vector<uint32_t> * offset_vector;

} serial_entry;


//save and load the mem tree
class serialise {

    private:
        //attributes
        std::vector<serial_entry> ptrchains_vector;

    //methods
    private:
        //methods
        void recurse_get_next_offset(mem_node * m_node, serial_entry * s_entry,
                                     uintptr_t last_point);
        void recurse_node(args_struct * args, mem * m, 
                          mem_node * m_node, unsigned int current_depth);

    public:
        //methods
        void tree_to_results(args_struct * args, mem * m, mem_tree * m_tree);
        void verify_ptrchains();

        void write_mem_ptrchains(args_struct * args, proc_mem * p_mem);
        void read_disk_ptrchains(args_struct * args);
};


#endif
