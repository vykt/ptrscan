#ifndef PROC_MEM_H
#define PROC_MEM_H

#include <vector>
#include <string>

#include <cstdio>

#include <unistd.h>

#include <libpwu.h>

#include "args.h"
#include "ui_base.h"


//indeces into satic_region_vct
#define STATIC_REGION_BSS 0
#define STATIC_REGION_STACK 1

//number of blacklisted segments
#define SEG_BLACKLIST_SIZE 2


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
        //void init_proc_mem(args_struct * args, ui_base * ui);
        void init_proc_mem(args_struct * args, ui_base * ui);

    private:
        //private methods
        //void fetch_pid(args_struct * args, ui_base * ui);
        void interpret_target(args_struct * args, ui_base * ui);
        void maps_init(maps_data * m_data);
        void add_static(args_struct * args, maps_entry * m_entry);
        void populate_regions(args_struct * args);
};

#endif
