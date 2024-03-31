#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <vector>

#include <libpwu.h>


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
#define MODE_RESCAN 3
#define MODE_RESCAN_WRITE 4

#define MODE_COUNT 5

/*
 *  In the future, consider allowing the user to specify a custom offset, probably
 *  as an exponent of 2.
 */


//additional region to treat as static in addition to .bss & [stack]
typedef struct {

    std::string pathname; //backing file
    int skip;             //if multiple regions with `pathname` backing file have
                          //been found, skip 'skip' first regions.
    //internal
    int skipped;

} region;


//arguments passed via flags
typedef struct {

    std::string target_str;
    std::string output_file;
    std::string input_file;
    byte ui_type;
    bool verbose;
    bool aligned;
    bool use_preset_offsets;
    uintptr_t ptr_lookback;
    uintptr_t target_addr;
    unsigned int levels;
    unsigned int num_threads;
    std::vector<region> extra_static_vector; //TODO rename to extra_static_vector
    std::vector<region> extra_rw_vector;
    std::vector<uintptr_t> preset_offsets;

} args_struct;

int process_args(int argc, char ** argv, args_struct * args);

#endif
