#include <vector>
#include <list>
#include <stdexcept>

#include <pthread.h>

#include "mem_tree.h"
#include "proc_mem.h"
#include "args.h"


static unsigned int next_node_id = 0;


//check if address is in read & write regions, or if it is in a static region (mode)
int check_index(uintptr_t node_addr, proc_mem * p_mem, int mode) {

    bool eval;
    std::vector<maps_entry *> * mode_vector;


    //get appropriate mode vector
    if (mode == CHECK_RW) {
        mode_vector = &p_mem->rw_regions_vector;
    } else if (mode == CHECK_STATIC) {
        mode_vector = &p_mem->static_regions_vector;
    }

    //for every static region
    for (unsigned int i = 0; i < (unsigned int) mode_vector->size();  ++i) {

        //check if node_addr falls in range of this static region
        eval = (uintptr_t) (*mode_vector)[i]->start_addr
               <= node_addr
               && (uintptr_t) (*mode_vector)[i]->end_addr
               > node_addr;

        //if it does fall inside this static region
        if (eval) {
            return i;
        }

    } //end for

    return -1;
}


//constructor for each node
mem_node::mem_node(uintptr_t node_addr, uintptr_t point_addr,
                   mem_node * parent_node, proc_mem * p_mem) {

    this->id = next_node_id;
    next_node_id++;

    this->node_addr = node_addr;
    this->point_addr = point_addr;
    this->rw_regions_index = check_index(node_addr, p_mem, CHECK_RW);
    this->static_regions_index = check_index(node_addr, p_mem, CHECK_STATIC);
    this->parent_node = parent_node;
}


//constructor, sets up root node of tree based on args
mem_tree::mem_tree(args_struct * args, proc_mem * p_mem) {

    const char * exception_str[1] = {
        "mem_tree -> constructor: failed to initialise write mutex."
    };

    int ret;

    //create root node
    this->root_node = new mem_node(args->target_addr, 0, NULL, p_mem);

    //create levels vector with space for args->levels number of lists
    this->levels = new std::vector<std::list<mem_node *>>(args->levels);

    //push the root node onto the first level
    (*this->levels)[0].push_back((mem_node *) this->root_node);

    //initialise the mutex
    ret = pthread_mutex_init(&this->write_mutex, NULL);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

}


//destructor to free the levels vector
mem_tree::~mem_tree() {

    delete this->levels;
    delete this->root_node;
    pthread_mutex_destroy(&this->write_mutex);
}


//add node to tree
void mem_tree::add_node(uintptr_t addr, uintptr_t point_addr, mem_node * parent_node, 
                        unsigned int level, proc_mem * p_mem) {

    const char * exception_str[2] = {
        "mem_tree -> add_node: failed to acquire write lock.",
        "mem_tree -> add_node: failed to release write lock."
    };

    int ret;
    std::list<mem_node *> * current_level_list;
    mem_node * pushed_mem_node;

    //define new node
    mem_node m_node(addr, point_addr, parent_node, p_mem);

    //acquire mutex to modify tree
    ret = pthread_mutex_lock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //add new node to its parent subnode_vector
    parent_node->subnode_list.push_front(m_node);


    //get meta list for current level
    current_level_list = &(*this->levels)[level];

    //put the just-inserted mem_node into the levels meta list
    pushed_mem_node = &parent_node->subnode_list.front();
    current_level_list->push_front(pushed_mem_node);

    //free mutex
    ret = pthread_mutex_unlock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[1]);
    }
}
