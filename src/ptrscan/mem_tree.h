#ifndef MEM_TREE_H
#define MEM_TREE_H

#include <vector>
#include <list>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "proc_mem.h"

/*
 *   Unlike UI part of the project, these classes use two stage initialisation to avoid
 *   dealing with the performance overhead of exceptions should an object fail to 
 *   instantialise.
 */

/*
 *   We need root -> leaf traversal for construction, and leaf -> root traversal for 
 *   using the tree.
 */

/* 
 *   if parent_node == NULL, this is a root node
 *   if is_static   == true, this is a valid leaf node
 */

//single node in tree
class mem_node {

    //attributes
    public:
    unsigned int id;
    int static_regions_index; //-1 if not static
    
    uintptr_t node_addr;                  //literal address of this node
    mem_node * parent_node;               //parents
    std::vector<mem_node> subnode_vector; //children


    //methods
    public:
    mem_node(uintptr_t node_addr, mem_node * parent_node, proc_mem * p_mem);
};


//memory tree
class mem_tree {

    //attributes
    public:
    std::vector<std::list<mem_node *>> * levels;
    const mem_node * root_node;

    //methods
    public:
    mem_tree(args_struct * args, proc_mem * p_mem);
    ~mem_tree();
};


#endif
