#ifndef SERIALISE_H
#define SERIALISE_H

#include <vector>

#include <cstdint>

#include "args.h"
#include "mem_tree.h"


#define DELIM_REGION 0
#define DELIM_OFFSET 0xFFFFFFF0

#define STATIC_REGIONS 0
#define RW_REGIONS 1
#define REGIONS_TYPE_NUM 2


//single set of offsets
typedef struct {

    uint32_t rw_regions_index;
    uint32_t static_regions_index;
    std::vector<uint32_t> * offset_vector;

} serial_entry;


//save and load the mem tree
class serialise {

    //attributes
    public:
    std::vector<std::string> rw_region_str_vector;
    std::vector<std::string> static_region_str_vector;
    std::vector<serial_entry> ptrchains_vector;

    //methods
    private:
    void recurse_get_next_offset(mem_node * m_node, serial_entry * s_entry,
                             uintptr_t last_point);
    void recurse_node(args_struct * args, mem_node * m_node, proc_mem * p_mem,
                      unsigned int current_level);

    public:
    void tree_to_results(args_struct * args, proc_mem * p_mem, mem_tree * m_tree);
    void verify_ptrchains();

    void write_mem_ptrchains(args_struct * args, proc_mem * p_mem);
    void read_disk_ptrchains(args_struct * args);
};


#endif
