#include <stdexcept>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <unistd.h>
#include <getopt.h>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"


//preset default arguments
static inline void _set_default_args(args_struct * args) {

    args->ui_type  = UI_TERM; 
    args->ln_iface = LN_IFACE_PROCFS;

    args->colour  = false;
    args->verbose = false;
    args->aligned = true;
    args->use_preset_offsets = false;

    args->bit_width   = 64;
    args->target_addr = 0;

    args->max_struct_size = 0x400;
    args->max_depth = 5;
    args->threads   = 1;

    return;
}


//convert optarg to an integer type
static uintptr_t _process_int_argument(const char * argument, 
                                       const char * exception_str) {

    uintptr_t temp;
    char * optarg_mod;
    int base;

    optarg_mod = optarg;
    base = 10;

    //check for null
    if (argument == nullptr) {
        throw std::runtime_error(exception_str);
    }

    //switch to base 16
    if (!strncmp(argument, "0x", 2)) {
        base = 16;
        optarg_mod += 2;
    }

    //try to convert to unsigned int
    try {
        temp = std::stol(argument, NULL, base);
    } catch(std::exception &e) {
        throw std::runtime_error(exception_str);
    }

    return temp;
}


//process offsets
static inline void _process_offsets(args_struct * args, char * offsets, 
                             const char * _exception_str) {

    const char * exception_str[1] {
        "process_offsets: failed to convert offset to uintptr_t."
    };

    char * next_offset_str;
    char * lookahead_comma;

    uintptr_t temp_offset;

    //throw exception if no argument passed
    if (optarg == nullptr) {
        throw std::runtime_error(_exception_str);
    }

    next_offset_str = offsets;

    //for every offset
    do {

        //convert string to uintptr_t
        temp_offset = _process_int_argument(next_offset_str, exception_str[0]);

        //add offset
        args->preset_offsets.insert(args->preset_offsets.end(), temp_offset);

        //get lookahead pointers
        lookahead_comma = strchr(next_offset_str, ',');
        next_offset_str = lookahead_comma + 1;

    } while (lookahead_comma != nullptr);

    return;
}


//process complex extra static regions argument (-s, --extra-static-regions)
static void _process_regions(std::vector<region> * region_vector, 
                             char * regions, const char * exception_str_) {

    //exception(s) for incorrect internal format of static regions
    const char * exception_str[1] {
        "process_regions: invalid region format."
    };

    char * next_region_str;
    char * lookahead_comma;
    char * lookahead_slash;

    region temp_region;


    //throw exception if no argument passed
    if (optarg == nullptr) {
        throw std::runtime_error(exception_str_);
    }

    //init
    next_region_str = regions;

    //for every region
    do {

        //get lookahead pointers
        lookahead_comma = strchr(next_region_str, ',');
        if (lookahead_comma == NULL) {
            throw std::runtime_error(exception_str[0]);
        }

        lookahead_slash = strchr(next_region_str, ':');

        //check if next_region_str doesn't have a comma 
        if (lookahead_slash != NULL && lookahead_comma > lookahead_slash) {
            throw std::runtime_error(exception_str[0]);
        }

        //build temp_region
        temp_region.pathname.assign(next_region_str, 
                                    lookahead_comma - next_region_str);

        temp_region.skip = atoi(lookahead_comma + 1);
        temp_region.skipped = 0;

        //add temp region
        region_vector->insert(region_vector->end(), temp_region);

    //while there are extra regions left to process
    } while ((next_region_str = 
            (lookahead_slash == NULL) ? NULL : lookahead_slash + 1) != NULL);
    return;
}


int process_args(int argc, char ** argv, args_struct * args) {

    const char * exception_str[] = {
        "process_args: use: -b <size:uint8_t> --bit-width=<size:uint8_t>",
        "process_args: use: -a <addr:uintptr_t> --target-addr=<addr:uintptr_t>",
        "process_args: use: -s <size:size_t> --max-struct-size=<size:size_t>",
        "process_args: use: -d <depth:uint> --max-depth=<depth:uint>",
        "process_args: use: -t <threads:uint> --threads=<threads:uint>",
        "process_args: use: -S <name:str>,<skip:int>:[...] --extra-static-regions=<name:str>,<skip:int>:[...]",
        "process_args: use: -R <name:str>,<skip:int>:[...] --exclusive-rw-regions=<name:str>,<skip:int>:[...]",
        "process_args: use: -O <off1:uintptr_t>,<off2:intptr_t>,[...] --preset-offsets=<off1:uintptr_t>,<off2:uintptr_t>,[...]",
        "process_args: use: ptrscan [flags] <target_name | target_pid>",
        "process_args: conflicting flags."
    };

    //defined cmdline options
    struct option long_opts[] = {
        {"output-file", required_argument, nullptr, 'w'},
        {"input-file", required_argument, nullptr, 'r'},
        {"ui-term", no_argument, nullptr, 'T'},
        {"ui-ncurses", no_argument, nullptr, 'N'},
        {"iface-procfs", no_argument, nullptr, 'p'},
        {"iface-lainko", no_argument, nullptr, 'k'},
        {"colour", no_argument, nullptr, 'c'},
        {"no-colour", no_argument, nullptr, 'n'},
        {"verbose", no_argument, nullptr, 'v'},
        {"aligned", no_argument, nullptr, 'A'},
        {"unaligned", no_argument, nullptr, 'U'},
        {"bit-width", required_argument, nullptr, 'b'},
        {"target-addr", required_argument, nullptr, 'a'},
        {"max-struct-size", required_argument, nullptr, 's'},
        {"max-depth", required_argument, nullptr, 'd'},
        {"threads", required_argument, nullptr, 't'},
        {"extra-static-regions", required_argument, nullptr, 'S'},
        {"exclusive-rw-regions", required_argument, nullptr, 'R'},
        {"preset-offsets", required_argument, nullptr, 'O'},
        {"verify", no_argument, nullptr, 'x'},
        {0,0,0,0}
    };

    int opt, opt_index;

    //mode array (use MACROs to index
    int mode_array[MODE_NUM] = {MODE_SCAN, MODE_SCAN_WRITE, MODE_READ, 
                                MODE_VERIFY, MODE_VERIFY_WRITE};

    //set default arguments
    _set_default_args(args);
    

    //iterate over supplied flags
    while(((opt = getopt_long(argc, argv,"w:r:TNpkcnvaub:p:s:d:t:S:R:O:x", 
           long_opts, &opt_index)) != -1) && (opt != 0)) {

        //determine parsed argument
        switch (opt) {

            case 'w': //output file
                args->output_file.assign(optarg);
                
                //eliminate modes based on flag
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_READ] = -1;
                mode_array[MODE_VERIFY] = -1;
                break;

            case 'r': //input file
                args->input_file.assign(optarg);

                //eliminate modes based on flag
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_SCAN_WRITE] = -1;
                break;

            case 'T': //terminal UI
                args->ui_type = UI_TERM;
                break;

            case 'N': //ncurses UI [not implemented]
                args->ui_type = UI_NCURSES;
                break;

            case 'p': //procfs interface
                args->ln_iface = LN_IFACE_PROCFS;
                break;

            case 'k': //lainko interface
                args->ln_iface = LN_IFACE_PROCFS;
                break;

            case 'c': //colour output
                args->colour = true;
                break;

            case 'n': //no colour output
                args->colour = false;
                break;

            case 'v': //verbose
                args->verbose = true;
                break;

            case 'A': //aligned pointer scan
                args->aligned = SCAN_ALIGNED;
                break;

            case 'U': //unaligned pointer scan
                args->aligned = SCAN_UNALIGNED;
                break;

            case 'b': //bit width
                args->bit_width = (cm_byte) _process_int_argument(optarg, 
                                                                  exception_str[0]);
                break;

            case 'a': //target address
                args->target_addr = (uintptr_t) 
                                    _process_int_argument(optarg, exception_str[1]);  
                break;

            case 's': //max struct size
                args->max_struct_size = 
                    (size_t) _process_int_argument(optarg, exception_str[2]);
                break;
            
            case 'd': //max depth
                args->max_depth = (unsigned int) 
                    _process_int_argument(optarg, exception_str[3]);
                break;
            
            case 't': //number of threads
                args->threads = 
                    (unsigned int) _process_int_argument(optarg, exception_str[4]);
                break;

            case 'S': //extra memory regions to treat as static
                _process_regions(&args->extra_static_regions, 
                                 optarg, exception_str[5]);
                break;

            case 'R': //exhaustive list of rw- regions to scan
                _process_regions(&args->exclusive_rw_regions, 
                                 optarg, exception_str[6]);
                break;

            case 'O': //specify preset offsets
                _process_offsets(args, optarg, exception_str[7]);
                args->use_preset_offsets = true;
                break;

            case 'x': //verify

                //eliminate non-rescan modes
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_SCAN_WRITE] = -1;
                mode_array[MODE_READ] = -1;
                
                break;

        } //end switch

    } //end opt while loop


    //fill remainder of preset_array with -1
    if (args->use_preset_offsets) {

        //for every remaining level without a preset offset
        for (unsigned int i = args->preset_offsets.size(); i < args->max_depth; ++i) {
            args->preset_offsets.insert(args->preset_offsets.end(), (uintptr_t) -1);
        }
    }

    //assign target string and check for null
    if (argv[optind] == 0) {
        throw std::runtime_error(exception_str[8]);
    }
    args->target_str.assign(argv[optind]);

    //return the earliest mode that is not eliminated
    for (int i = 0; i < MODE_NUM; ++i) {
        if (mode_array[i] != -1) return mode_array[i];
    } //end for
    
    //otherwise, throw an exception
    throw std::runtime_error(exception_str[8]);
    return -1;
}
