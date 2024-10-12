#include <vector>
#include <list>
#include <stdexcept>

#include <pthread.h>

#include "mem_tree.h"
#include "mem.h"
#include "args.h"


static int _next_node_id = 0;


// --- PRIVATE mem_node METHODS

//check if address is in read & write regions, or if it is in a static region (mode)
const int mem_node::check_index(const mem * m, const int mode) {

    bool eval;

    ln_vm_area * vma;
    const std::vector<cm_list_node *> * mode_vector;


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
        if ((vma->start_addr <= this->addr) && (vma->end_addr > this->addr)) return i;

    } //end for

    return -1;
}


// --- PUBLIC mem_node METHODS

//constructor for each node
mem_node::mem_node(const uintptr_t addr, const uintptr_t ptr_addr, 
                   const cm_list_node * vma_node, const mem_node * parent, 
                   const mem * m) :
    
    //initialiser list
    id(_next_node_id),
    rw_regions_index(check_index(m, CHECK_RW)),
    static_regions_index(check_index(m, CHECK_STATIC)),
    addr(addr),
    ptr_addr(ptr_addr),
    vma_node(vma_node),
    parent(parent) 
    //end initialiser list
    
    {

    _next_node_id++;

    return;
}


inline const mem_node * mem_node::add_child(const mem_node * child) {
    this->children.push_front(*child);
    return &this->children.front();
}


inline const int mem_node::get_rw_regions_index() const {
    return this->rw_regions_index;
}


inline const int mem_node::get_static_regions_index() const {
    return this->static_regions_index;
}


inline const uintptr_t mem_node::get_addr() const {
    return this->addr;
}


inline const uintptr_t mem_node::get_ptr_addr() const {
    return this->ptr_addr;
}


inline const cm_list_node * mem_node::get_vma_node() const {
    return this->vma_node;
}


inline const mem_node * mem_node::get_parent() const {
    return this->parent;
}


inline std::list<mem_node> * mem_node::get_children() {
    return &this->children;
}



// --- PUBLIC mem_tree METHODS

//constructor, sets up root node of tree based on args
mem_tree::mem_tree(const args_struct * args, mem * m) {

    const char * exception_str[2] = {
        "mem_tree -> constructor: target address does not belong to a vma."
        "mem_tree -> constructor: failed to initialise write mutex."
    };

    int ret;
    cm_list_node * vma_node;


    //locate vma of root node
    vma_node = ln_get_vm_area_by_addr(m->get_map(), args->target_addr, nullptr);
    if (vma_node == NULL) {
        throw std::runtime_error(exception_str[0]);
    }

    //create root node
    this->root_node = new mem_node(args->target_addr, 0, vma_node, nullptr, m);

    //create tree levels vector to store a lists of nodes at each depth level
    this->levels = new std::vector<std::list<mem_node *>>(args->max_depth);

    //push the root node onto the first level
    (*this->levels)[0].push_back((mem_node *) this->root_node);

    //initialise the mutex
    ret = pthread_mutex_init(&this->write_mutex, nullptr);
    if (ret) {
        throw std::runtime_error(exception_str[1]);
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
void mem_tree::add_node(const uintptr_t addr, const uintptr_t ptr_addr, 
                        const cm_list_node * vma_node, mem_node * parent, 
                        const int level, const mem * m) {

    const char * exception_str[2] = {
        "mem_tree -> add_node: failed to acquire write lock.",
        "mem_tree -> add_node: failed to release write lock."
    };

    int ret;

    std::list<mem_node *> * current_level_list;
    const mem_node * pushed_m_node;


    //define new node
    mem_node m_node(addr, ptr_addr, vma_node, parent, m);

    //acquire mutex to modify tree
    ret = pthread_mutex_lock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //add new node to its parent subnode_vector
    pushed_m_node = parent->add_child(&m_node);

    //get meta list for current level
    current_level_list = &(*this->levels)[level];

    //put the just-inserted mem_node into the levels meta list
    current_level_list->push_front((mem_node *) pushed_m_node);

    //free mutex
    ret = pthread_mutex_unlock(&this->write_mutex);
    if (ret) {
        throw std::runtime_error(exception_str[1]);
    }

    return;
}


inline std::list<mem_node *> * mem_tree::get_level_list(int level) const {
    return &(*this->levels)[level];
}


inline const mem_node * mem_tree::get_root_node() const {
    return this->root_node;
}
