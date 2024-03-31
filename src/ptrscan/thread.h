#ifndef THREAD_H
#define THREAD_H

#include <vector>

#include <cstdint>

#include <libpwu.h>
#include <pthread.h>

#include "args.h"
#include "ui_base.h"
#include "mem_tree.h"


#define READ_BUF_SIZE 0x1000 //maybe set to sizeof(memory page) later


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
    public:
    int human_thread_id;
    unsigned int * current_level; //points to thread controller's current_level
    pthread_t id;
    pthread_barrier_t * level_barrier; //points to thread_ctrl member
    
    int mem_fd; //fd for /proc/mem, opened by thread_ctrl
    uintptr_t current_addr;
    
    std::vector<parent_range> * parent_range_vector; //points to thread_ctrl member 
    std::vector<mem_range> regions_to_scan; //initialised separately by thread_ctrl
    

    //methods
    private:
    int addr_parent_compare(uintptr_t addr, args_struct * args);

    public:
    ssize_t get_next_buffer_smart(byte * mem_buf, ssize_t read_left, 
                                 ssize_t read_last, bool first_region_read);
    void thread_main(args_struct * args, proc_mem * p_mem, mem_tree * m_tree,
                     ui_base * ui);

    void reset_current_addr();
    
};


//struct to pass 'global' objects + own thread object to the created thread
typedef struct {

    thread * t;
    args_struct * args;
    proc_mem * p_mem;
    mem_tree * m_tree;
    ui_base * ui;

} thread_arg;

//function called by pthread_create_t
void * thread_bootstrap(void * arg_bootstrap);


#endif
