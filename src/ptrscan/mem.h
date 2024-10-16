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
        const pid_t pid;
        
        ln_session session;
        ln_vm_map map;

        std::vector<cm_list_node *> rw_areas;     //holds: ln_vm_area
        std::vector<cm_list_node *> static_areas; //holds: ln_vm_area


    private:
        //methods
        const pid_t interpret_target(args_struct * args);
        void open_session(const args_struct * args);
        void close_session();
        void acquire_map();
        void release_map();
        void add_standard_vmas(args_struct * args);
        void add_static_vma(args_struct * args, cm_list_node * vma_node);

    public:
        //methods
        mem(args_struct * args);
        ~mem();
        void populate_areas(args_struct * args);

        //getters & setters
        const int get_pid() const;
        const ln_session * get_session() const;
        const ln_vm_map * get_map() const;
        const std::vector<cm_list_node *> * get_rw_areas() const;
        const std::vector<cm_list_node *> * get_static_areas() const;
};

#endif
