#include <vector>
#include <list>

#include "mem_tree.h"
#include "proc_mem.h"
#include "args.h"


static unsigned int next_node_id = 0;


//check if address is static
int check_static(uintptr_t node_addr, proc_mem * p_mem) {

    bool eval;
    
    //for every static region
    for (int i = 0; i < p_mem->static_regions_vector.size(); ++i) {

        //check if node_addr falls in range of this static region
        eval = (uintptr_t) p_mem->static_regions_vector[i]->start_addr
               <= node_addr
               && (uintptr_t) p_mem->static_regions_vector[i]->end_addr
               > node_addr;

        //if it does fall inside this static region
        if (eval) {
            return i;
        }

    } //end for

    return -1;
}


//constructor for each node
mem_node::mem_node(uintptr_t node_addr, mem_node * parent_node, proc_mem * p_mem) {

    this->id = next_node_id;
    next_node_id++;

    this->node_addr = node_addr;
    this->static_regions_index = check_static(node_addr, p_mem);
    this->parent_node = parent_node;
}


//constructor, sets up root node of tree based on args
mem_tree::mem_tree(args_struct * args, proc_mem * p_mem) {

    //create root node
    this->root_node = new mem_node(args->target_addr, NULL, p_mem);

    //create levels vector with space for args->levels number of lists
    this->levels = new std::vector<std::list<mem_node *>>(args->levels);

    //push the root node onto the first level
    (*this->levels)[0].push_back((mem_node *) this->root_node);
}


//destructor to free the levels vector
mem_tree::~mem_tree() {

    delete this->levels;
    delete this->root_node;
}
