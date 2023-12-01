#ifndef MEM_TREE_H
#define MEM_TREE_H

#include <vector>

#include <cstdint>

#include <pthread.h>


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
    const unsigned int id;
    
    private:
    bool is_static;
    uintptr_t node_addr;
    mem_node * parent_node;
    std::vector<mem_node> subnode_vector;


    //methods
    public:
    mem_node(); //TODO finish constructor
};


//memory tree
class mem_tree {

    //attributes
    private:
    std::vector<std::vector<mem_node *>> levels;
    pthread_mutex_t m_lock;

    //methods
    public:
    mem_tree(); //TODO finish constructor
};


#endif
