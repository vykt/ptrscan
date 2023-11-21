#ifndef PROC_MEM_H
#define PROC_MEM_H

#include <vector>
#include <string>

#include <libpwu.h>

#include "ui.h"


//indeces into satic_region_vct
#define STATIC_REGION_BSS 0
#define STATIC_REGION_STACK 1


//target process memory class
class proc_mem {

    public:
        maps_data m_data;
        std::vector<maps_entry *> rw_regions_vec;
        std::vector<maps_entry *> static_regions_vec;

        proc_mem(std::string target_str, byte flags,
                 std::vector<extra_static_region> * esr_vec = nullptr);
};

#endif
