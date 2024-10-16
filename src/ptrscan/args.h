#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <vector>

#include <libcmore.h>
#include <liblain.h>


#define UI_TERM 0
#define UI_NCURSES 1

#define SCAN_ALIGNED true
#define SCAN_UNALIGNED false

#define STRTYPE_INT 0
#define STRTYPE_LONG 1
#define STRTYPE_LLONG 2


//these could be reported back to main in a clever way (bitmask?)
#define MODE_SCAN 0
#define MODE_SCAN_WRITE 1
#define MODE_READ 2
#define MODE_VERIFY 3
#define MODE_VERIFY_WRITE 4

#define MODE_NUM 5


//additional region to treat as static in addition to .bss & [stack]
typedef struct {

    std::string pathname; //backing object
    int skip;             //if multiple backing objects with the [path]name have 
                          //been found, skip first 'skip' objects.
    //internal
    int skipped;

} region;


/*
 *  This struct effectively stores the runtime configuration. It is first populated
 *  by argument parsing code in args.cpp, and then by the initialisation in mem.cpp.
 */

//arguments passed via flags
typedef struct {

    //supplied by user
    std::string target_str;    //target name (comm)
    std::string output_file;
    std::string input_file;

    cm_byte ui_type;           //not implemented
    int ln_iface;

    bool colour;               //use colour codes in output 
    bool verbose;
    bool aligned;
    bool use_preset_offsets;   //did user supply first n offsets?
    
    cm_byte bit_width;         //architecture bit width (32bit/64bit, etc.)
    uintptr_t target_addr;     //root node address
    
    size_t max_struct_size;    //max struct size
    unsigned int max_depth;    //max tree levels
    unsigned int threads;      //number of worker threads to use
    
    std::vector<region> extra_static_areas;  //user supplied vmas to treat as static
    std::vector<region> exclusive_rw_areas;  //user supplied vmas to exclusively scan
    std::vector<uint32_t> preset_offsets;     //first n offsets (if supplied)

    //internal
    std::string target_comm;

} args_struct;

int process_args(int argc, char ** argv, args_struct * args);

#endif
