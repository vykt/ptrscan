#ifndef THREADS_H
#define THREADS_H

#include <vector>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "mem_tree.h"


//a region that points to one of the parent nodes
typedef struct {

    mem_node * parent_node;
    uintptr_t start_addr;
    uintptr_t end_addr;

} parent_range;


//one of the regions a thread is set to scan
typedef struct {

    maps_entry * m_entry;
    uintptr_t start_addr; //can be different from m_entry->start_addr
    uintptr_t end_addr;   //can be different from m_entry->end_addr

} mem_range;


//data for one thread
class thread {

    pthread_t id;
    std::vector<mem_range> regions_to_scan;
    uintptr_t current_addr;

};


//thread controller, 'global state'
class thread_ctrl {

    //attributes
    private:
    std::vector<thread> thread_vector;
    unsigned int current_level;

};


#endif
