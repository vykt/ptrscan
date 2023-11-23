#ifndef PROC_MEM_H
#define PROC_MEM_H

#include <vector>
#include <string>

#include <cstdio>

#include <unistd.h>

#include <libpwu.h>

#include "ui_base.h"


//indeces into satic_region_vct
#define STATIC_REGION_BSS 0
#define STATIC_REGION_STACK 1

//region memory permissions mask



//target process memory class
class proc_mem {

    public:
        //public attributes
        pid_t pid;
        int mem_fd;
        FILE * maps_stream;

        maps_data m_data;
        std::vector<maps_entry *> rw_regions_vector;
        std::vector<maps_entry *> static_regions_vector;

        //public methods
        proc_mem(std::string target_str, byte flags,
                 std::vector<static_region> * extra_static_vector = nullptr);

    private:
        //private methods
        void fetch_pid(std::string target_str);
        void maps_init(maps_data * m_data);
        void add_static(std::vector<static_region> * static_vector,
                        maps_entry * m_entry);
        void populate_regions(std::vector<static_region> * extra_static_vector,
                              std::string target_str);
};

#endif
