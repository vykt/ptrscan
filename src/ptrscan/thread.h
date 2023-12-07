#ifndef THREAD_H
#define THREAD_H

#include <vector>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "args.h"
#include "mem_tree.h"


//one of the regions a thread is set to scan
typedef struct {

    maps_entry * m_entry;
    uintptr_t start_addr;
    uintptr_t end_addr;

} mem_range;


//a region that is deemed to point to one of the parent nodes
typedef struct {

    mem_node * parent_node;
    uintptr_t start_addr;
    uintptr_t end_addr;

} parent_range;


//class representing one thread
class thread {

    //attributes
    private:
    pthread_barrier_t * level_barrier;               //points to thread_ctrl member
    std::vector<parent_range> * parent_range_vector; //points to thread_ctrl member 

    uintptr_t current_addr;
    bool finished_level;

    public:
    pthread_t id;                            //accessed internally frequently
    std::vector<mem_range> regions_to_scan;  //initialised separately by thread_ctrl
    

    //methods
    public:
    thread(pthread_barrier_t * level_barrier,
           std::vector<parent_range> * parent_range_vector);
    int thread_main(args_struct * args);

    void reset_current_addr();
    int confirm_finished_level();  //fail if not yet finished, reset if finished
    
};


//struct to pass 'global' objects + own thread object to the created thread
typedef struct {

    thread * t;
    args_struct * args;
    proc_mem * p_mem;
    mem_tree * m_tree;

} thread_arg;

//function called by pthread_create_t
void * thread_bootstrap(void * arg_bootstrap);


#endif
