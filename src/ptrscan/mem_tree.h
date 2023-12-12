#ifndef MEM_TREE_H
#define MEM_TREE_H

#include <vector>
#include <list>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "proc_mem.h"


/*
 *   We need root -> leaf traversal for construction, and leaf -> root traversal for 
 *   using the tree.
 */

/*
 *   The mem_node should probably be a struct.
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
    std::list<mem_node> subnode_list;     //children


    //methods
    public:
    mem_node(uintptr_t node_addr, mem_node * parent_node, proc_mem * p_mem);
};


//memory tree
class mem_tree {

    //attributes
    private:
    pthread_mutex_t write_mutex;

    public:
    std::vector<std::list<mem_node *>> * levels;
    const mem_node * root_node;

    //methods
    public:
    mem_tree(args_struct * args, proc_mem * p_mem);
    ~mem_tree();

    void add_node(uintptr_t addr, mem_node * parent_node, 
                  unsigned int level, proc_mem * p_mem);
};


#endif
