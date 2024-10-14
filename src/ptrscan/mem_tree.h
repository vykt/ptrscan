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
        const int id;

        const int rw_areas_index;     //-1 if not in a rw region
        const int static_areas_index; //-1 if not in a static region

        const uintptr_t addr;           //where this pointer is stored
        const uintptr_t ptr_addr;       //where this pointer points to

        const cm_list_node * vma_node;

        const mem_node * parent;
        std::list<mem_node> children;

    private:
        //methods
        const int check_index(const mem * m, const int mode);

    public:
        //methods
        mem_node(const uintptr_t addr, const uintptr_t ptr_addr, 
                 const cm_list_node * vma_node, const mem_node * parent, 
                 const mem * m);

        const mem_node * add_child(const mem_node * child);

        //getters & setters
        const int get_rw_areas_index() const;
        const int get_static_areas_index() const;
        
        const uintptr_t get_addr() const;
        const uintptr_t get_ptr_addr() const;
        const cm_list_node * get_vma_node() const;

        const mem_node * get_parent() const;
        const std::list<mem_node> * get_children() const;
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
        mem_tree(const args_struct * args, mem * m);
        ~mem_tree();

        void add_node(const uintptr_t addr, const uintptr_t ptr_addr, 
                      const cm_list_node * vma_node, mem_node * parent, 
                      const int level, const mem * m);

        //getters & setters
        std::list<mem_node *> * get_level_list(int level) const;
        const mem_node * get_root_node() const;
};


#endif
