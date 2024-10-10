#ifndef MEM_TREE_H
#define MEM_TREE_H

#include <vector>
#include <list>

#include <cstdint>

#include <pthread.h>
#include <libcmore.h>
#include <liblain.h>

#include "mem.h"


#define CHECK_RW 0
#define CHECK_STATIC 1



/*
 *   We need root -> leaf traversal for construction, and leaf -> root traversal 
 *   when verifying or using the tree.
 */

/* 
 *   if parent_node == NULL, this is a root node
 *   if is_static   == true, this is a valid leaf node
 */

//single node in tree
class mem_node {

    private:
        //attributes
        unsigned int id;

        int rw_regions_index;     //-1 if not in a rw region
        int static_regions_index; //-1 if not in a static region

        uintptr_t addr;           //where this pointer is stored
        uintptr_t ptr_addr;       //where this pointer points to
        
        mem_node * parent;
        std::list<mem_node> children;

    public:
        //methods
        mem_node(uintptr_t addr, uintptr_t ptr_addr, mem_node * parent, mem * m);

        void add_child(mem_node * child);

        mem_node * get_parent();
        std::list<mem_node> * get_children();
};


//memory tree
class mem_tree {

    private:
        //attributes
        pthread_mutex_t write_mutex;
        std::vector<std::list<mem_node *>> * levels; //all levels of the tree
        const mem_node * root_node;

    public:
        //methods
        mem_tree(args_struct * args, mem * m);
        ~mem_tree();

        void add_node(uintptr_t addr, uintptr_t ptr_addr, 
                      mem_node * parent, unsigned int level, mem * m);
};


#endif
