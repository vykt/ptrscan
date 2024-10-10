#ifndef MEM_H
#define MEM_H

#include <vector>
#include <string>

#include <cstdio>

#include <unistd.h>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "ui_base.h"


//indeces into satic_region_vct
#define STATIC_REGION_BSS 0
#define STATIC_REGION_STACK 1

//number of blacklisted segments
#define VMA_BLACKLIST_SIZE 3


//target process memory class
class mem {

    private:
        //attributes
        pid_t pid;
        
        ln_session session;
        ln_vm_map map;

        std::vector<cm_list_node *> rw_regions;     //holds: ln_vm_area
        std::vector<cm_list_node *> static_regions; //holds: ln_vm_area

    public:
        //methods
        void init_mem(args_struct * args, ui_base * ui);
        
        std::vector<cm_list_node *> * get_rw_regions();
        std::vector<cm_list_node *> * get_static_regions();

    private:
        //methods
        void interpret_target(args_struct * args);
        void open_session(args_struct * args);
        void get_map();
        void add_standard_vmas(args_struct * args);
        void add_static_vma(args_struct * args, cm_list_node * vma_node);
        void populate_regions(args_struct * args);
};

#endif
