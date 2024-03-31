#include <iostream>
#include <stdexcept>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <unistd.h>
#include <getopt.h>

#include <libpwu.h>

#include "args.h"
#include "ui_base.h"


//process integer arguments 
inline uintptr_t process_int_argument(const char * argument, const char * exception_str) {

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
void process_offsets(args_struct * args, char * offsets, const char * exception_str_) {

    const char * exception_str[1] {
        "process_offsets: failed to convert offset to uintptr_t."
    };

    char * next_offset_str;
    char * lookahead_comma;

    uintptr_t temp_offset;

    //throw exception if no argument passed
    if (optarg == nullptr) {
        throw std::runtime_error(exception_str_);
    }

    next_offset_str = offsets;

    //for every offset
    do {

        //convert string to uintptr_t
        temp_offset = process_int_argument(next_offset_str, exception_str[0]);

        //add offset
        args->preset_offsets.insert(args->preset_offsets.end(), temp_offset);

        //get lookahead pointers
        lookahead_comma = strchr(next_offset_str, ',');
        next_offset_str = lookahead_comma + 1;

    } while (lookahead_comma != nullptr);
}


//process complex extra static regions argument (-s, --extra-static-regions)
void process_regions(args_struct * args, std::vector<region> * region_vector, 
                            char * regions, const char * exception_str_) {

    //exception(s) for incorrect internal format of static regions
    const char * exception_str[1] {
        "process_extra_static_regions: invalid static region format."
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
    } while((next_region_str = 
            (lookahead_slash == NULL) ? NULL : lookahead_slash + 1) != NULL);
}


int process_args(int argc, char ** argv, args_struct * args) {

    const char * exception_str[] = {
        "process_args: use: -p <lookback:int> --ptr-lookback=<lookback:int>",
        "process_args: use: -l <depth> --levels=<depth>",
        "process_args: use: -s <name:str>,<skip:int> --extra-static-regions=<name:str>,<skip:int>:[...]",
        "process_args: use: -r <name:str>,<skip:int> --define-rw-regions=<name:str>,<skip:int>:[...]",
        "process_args: use: -o <off1>,<off2>,[...] --offsets=<off1>,<off2>,[...]",
        "process_args: use: -t <thread_num> --threads=<thread_num>",
        "process_args: use: -a <addr> --target-addr=<addr>",
        "process_args: use: ptrscan [flags] <target_name | target_pid>",
        "process_args: no valid mode remaining."
    };

    //defined cmdline options
    struct option long_opts[] = {
        {"ui-term", no_argument, nullptr, 'c'},
        {"ui-ncurses", no_argument, nullptr, 'n'},
        {"verbose", no_argument, nullptr, 'v'},
        {"ptr-lookback", required_argument, nullptr, 'p'},
        {"levels", required_argument, nullptr, 'l'},
        {"extra-static-regions", required_argument, NULL, 's'},
        {"define-rw-regions", required_argument, NULL, 'e'},
        {"offsets", required_argument, NULL, 'o'},
        {"aligned", no_argument, NULL, 'q'},
        {"unaligned", no_argument, NULL, 'u'},
        {"threads", required_argument, NULL, 't'},
        {"output-file", required_argument, NULL, 'w'},
        {"input-file", required_argument, NULL, 'r'},
        {"verify", no_argument, NULL, 'x'},
        {"target-addr", required_argument, NULL, 'a'},
        {0,0,0,0}
    };

    int opt, opt_index;

    //mode array (use MACROs to index
    int mode_array[MODE_COUNT] = {MODE_SCAN, MODE_SCAN_WRITE, MODE_READ, 
                                  MODE_RESCAN, MODE_RESCAN_WRITE};

    //set defaults
    args->ui_type = UI_TERM;
    args->ptr_lookback = 0x400;
    args->target_addr = 0;
    args->levels = 5;
    args->aligned = true;
    args->verbose = false;
    args->use_preset_offsets = false;
    args->num_threads = 1;

    //option processing while loop
    while((opt = getopt_long(argc, argv, "cnvp:l:s:r:o:qut:w:e:xa:", 
           long_opts, &opt_index)) != -1 
          && opt != 0) {

        //determine parsed argument
        switch (opt) {

            case 'c': //terminal UI
                args->ui_type = UI_TERM;
                break;

            case 'n': //ncurses UI
                args->ui_type = UI_NCURSES;
                break;

            case 'v': //verbose
                args->verbose = true;
                break;

            case 'p': //pointer lookback
                args->ptr_lookback = 
                    (uintptr_t) process_int_argument(optarg, exception_str[0]);
                break;
            
            case 'l': //depth level
                args->levels = (unsigned int) process_int_argument(optarg, exception_str[1]);
                break;
            
            case 's': //extra memory regions to treat as static
                process_regions(args, &args->extra_static_vector, optarg, exception_str[2]);
                break;

            case 'e': //exhaustive list of rw- regions to scan
                process_regions(args, &args->extra_rw_vector, optarg, exception_str[3]);
                break;

            case 'o': //specify preset offsets
                process_offsets(args, optarg, exception_str[4]);
                args->use_preset_offsets = true;
                break;

            case 'q': //aligned pointer scan
                args->aligned = SCAN_ALIGNED;
                break;

            case 'u': //unaligned pointer scan
                args->aligned = SCAN_UNALIGNED;
                break;

            case 't': //number of threads
                args->num_threads = 
                    (unsigned int) process_int_argument(optarg, exception_str[5]);
                break;

            case 'w': //output file
                args->output_file.assign(optarg);
                
                //eliminate modes based on flag
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_READ] = -1;
                mode_array[MODE_RESCAN] = -1;

                break;

            case 'r': //input file
                args->input_file.assign(optarg);

                //eliminate modes based on flag
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_SCAN_WRITE] = -1;

                break;

            case 'x': //rescan

                //eliminate modes based on flag
                mode_array[MODE_SCAN] = -1;
                mode_array[MODE_SCAN_WRITE] = -1;
                mode_array[MODE_READ] = -1;
                
                break;

            case 'a': //target address
                args->target_addr = (uintptr_t) process_int_argument(optarg, exception_str[6]); 
                break;

        } //end switch

    } //end opt while loop


    //fill remainder of preset_array with -1
    if (args->use_preset_offsets) {

        //for every remaining level without a preset offset
        for (unsigned int i = args->preset_offsets.size(); i < args->levels; ++i) {
            args->preset_offsets.insert(args->preset_offsets.end(), (uintptr_t) -1);
        }
    }

    //assign target string and check for null
    if (argv[optind] == 0) {
        throw std::runtime_error(exception_str[7]);
    }
    args->target_str.assign(argv[optind]);

    //return the earliest mode that is not eliminated
    for (int i = 0; i < MODE_COUNT; ++i) {
        if (mode_array[i] != -1) return mode_array[i];
    } //end for
    
    //otherwise, throw an exception
    throw std::runtime_error(exception_str[8]);
    return -1;
}
