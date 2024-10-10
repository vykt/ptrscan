#include <vector>
#include <list>
#include <stdexcept>

#include <pthread.h>

#include "mem_tree.h"
#include "proc_mem.h"
#include "args.h"


static unsigned int _next_node_id = 0;


// --- INTERNAL

//check if address is in read & write regions, or if it is in a static region (mode)
static int _check_index(uintptr_t addr, mem * m, int mode) {

    bool eval;

    ln_vm_area * vma;
    std::vector<cm_list_node *> * mode_vector;


    //get appropriate mode vector
    if (mode == CHECK_RW) {
        mode_vector = m->get_rw_regions();
    } else if (mode == CHECK_STATIC) {
        mode_vector = m->get_static_regions();
    }

    //for every region
    for (int i = 0; i < mode_vector->size();  ++i) {

        vma = LN_GET_NODE_AREA((*mode_vector)[i]);

        //check if node_addr falls in range of this static region
        if ((vma->start_addr <= addr) && (vma->end_addr > addr)) return i;

    } //end for

    return -1;
}


// --- PUBLIC mem_node METHODS

//constructor for each node
mem_node::mem_node(uintptr_t addr, uintptr_t ptr_addr, 
                   mem_node * parent, mem * m) {

    this->id = _next_node_id;
    _next_node_id++;

    this->addr                 = addr;
    this->ptr_addr             = ptr_addr;
    this->rw_regions_index     = _check_index(addr, m, CHECK_RW);
    this->static_regions_index = _check_index(addr, m, CHECK_STATIC);
    this->parent               = parent;

    return;
}


inline void mem_node::add_child(mem_node * child) {
    this->children.push_front(*child);
    return;
}


inline mem_node * mem_node::get_parent() {
    return this->parent;
}


inline std::list<mem_node> * mem_node::get_children() {
    return &this->children;
}



// --- PUBLIC mem_tree METHODS

//constructor, sets up root node of tree based on args
mem_tree::mem_tree(args_struct * args, mem * m) {

    const char * exception_str[1] = {
        "mem_tree -> constructor: failed to initialise write mutex."
    };

    int ret;

    //create root node
    this->root_node = new mem_node(args->target_addr, 0, nullptr, m);

    //create tree levels vector to store a lists of nodes at each depth level
    this->levels = new std::vector<std::list<mem_node *>>(args->max_depth);

    //push the root node onto the first level
    (*this->levels)[0].push_back((mem_node *) this->root_node);

    //initialise the mutex
    ret = pthread_mutex_init(&this->write_mutex, nullptr);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


//destructor to free the levels vector
mem_tree::~mem_tree() {

    delete this->levels;
    delete this->root_node;
    pthread_mutex_destroy(&this->write_mutex);
}


//add node to tree
void mem_tree::add_node(uintptr_t addr, uintptr_t ptr_addr, 
                        mem_node * parent, unsigned int level, mem * m) {

    const char * exception_str[2] = {
        "mem_tree -> add_node: failed to acquire write lock.",
        "mem_tree -> add_node: failed to release write lock."
    };

    int ret;

    std::list<mem_node> * parent_children_list;
    std::list<mem_node *> * current_level_list;
    mem_node * pushed_m_node;


    //define new node
    mem_node m_node(addr, ptr_addr, parent, m);

    //acquire mutex to modify tree
    ret = pthread_mutex_lock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //add new node to its parent subnode_vector
    parent->add_child(&m_node);

    //get parent's children list
    parent_children_list = parent->get_children();

    //get meta list for current level
    current_level_list = &(*this->levels)[level];

    //put the just-inserted mem_node into the levels meta list
    pushed_m_node = &parent_children_list->front();
    current_level_list->push_front(pushed_m_node);

    //free mutex
    ret = pthread_mutex_unlock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[1]);
    }
}
