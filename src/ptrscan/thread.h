#ifndef THREAD_H
#define THREAD_H

#include <vector>

#include <cstdint>

#include <pthread.h>
#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "ui_base.h"
#include "mem_tree.h"


#define READ_BUF_SIZE 0x1000 //maybe set to sizeof(memory page) later


//one of the regions a thread is set to scan
typedef struct {

    cm_list_node * vma_node;
    uintptr_t start_addr;
    uintptr_t end_addr;

} vma_scan_range;


//a range of addresses accepted to point to a parent (parent addr - max struct size)
typedef struct {

    mem_node * parent_node;
    uintptr_t start_addr;
    uintptr_t end_addr;

} parent_range;


//class representing one thread
class thread {

    private:
        //attributes
        int ui_id;
        pthread_t id;
        pthread_barrier_t * depth_barrier; //ptr to thread_ctrl's member
        
        unsigned int * current_depth;      //ptr to thread_ctrl's current_level
        uintptr_t current_addr;

        ln_session session;                //this thread's exclusive session
        
        std::vector<parent_range> * parent_ranges;   //ptr to thread_ctrl's member 
        std::vector<vma_scan_range> vma_scan_ranges; //initialised by thread_ctrl
    
    private:
        //methods
        int addr_parent_compare(uintptr_t addr, args_struct * args);

    public:
        //methods
        ssize_t get_next_buffer_smart(cm_byte * mem_buf, ssize_t read_left, 
                                      ssize_t read_last, bool first_region_read);
        void thread_main(args_struct * args, mem * m, mem_tree * m_tree, ui_base * ui);
        void reset_current_addr();
        
        void link_thread(int ui_id, unsigned int * current_depth,
                         pthread_barrier_t * depth_barrier, 
                         std::vector<parent_range> * parent_ranges);
        void setup_session(int ln_iface, int pid);

        pthread_t * get_id();

        int get_ui_id();
        void set_ui_id(int ui_id);

        std::vector<vma_scan_range> * get_vma_scan_ranges();
};


//struct to pass 'global' objects + own thread object to the created thread
typedef struct {

    thread * t;
    args_struct * args;
    mem * m;
    mem_tree * m_tree;
    ui_base * ui;

} thread_arg;

//function called by pthread_create_t
void * thread_bootstrap(void * arg_bootstrap);


#endif
