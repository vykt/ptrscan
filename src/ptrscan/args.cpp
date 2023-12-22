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


//TODO fuzz me


//process complex extra static regions argument (-s, --extra-static-regions)
inline void process_extra_static_regions(args_struct * args, char * regions,
                                         const char * exception_str_) {

    //exception(s) for incorrect internal format of static regions
    const char * exception_str[1] {
        "process_extra_static_regions: invalid static region format."
    };

    char * next_region_str;
    char * lookahead_comma;
    char * lookahead_slash;

    static_region temp_region;


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
        args->extra_region_vector.insert(args->extra_region_vector.end(), temp_region);

    //while there are extra regions left to process
    } while((next_region_str = 
            (lookahead_slash == NULL) ? NULL : lookahead_slash + 1) != NULL);
}


//process integer arguments 
inline uintptr_t process_int_argument(const char * exception_str) {

    uintptr_t temp;
    char * optarg_mod;
    int base;

    optarg_mod = optarg;
    base = 10;

    //check for null
    if (optarg == nullptr) {
        throw std::runtime_error(exception_str);
    }

    //switch to base 16
    if (!strncmp(optarg, "0x", 2)) {
        base = 16;
        optarg_mod += 2;
    }

    //try to convert to unsigned int
    try {
        temp = std::stol(optarg, NULL, base);
    } catch(std::exception &e) {
        throw std::runtime_error(exception_str);
    }

    return temp;
}


void process_args(int argc, char ** argv, args_struct * args) {

    const char * exception_str[6] = {
        "process_args: use: -p <lookback:int> --ptr-lookback=<lookback:int>",
        "process_args: use: -l <depth> --levels=<depth>",
        "process_args: use: -s <name:str>,<skip:int> \
--extra-static-regions=<name:str>,<skip:int>:<...>",
        "process_args: use: -t <thread_num> --threads=<thread_num>",
        "process_args: use: -a <addr> --target-addr=<addr>",
        "process_args: use: ptrscan [flags] <target_name>"
    };

    //defined cmdline options
    struct option long_opts[] = {
        {"ui-term", no_argument, nullptr, 'c'},
        {"ui-ncurses", no_argument, nullptr, 'n'},
        {"ptr-lookback", required_argument, nullptr, 'p'},
        {"levels", required_argument, nullptr, 'l'},
        {"extra-static-regions", required_argument, NULL, 's'},
        {"aligned", no_argument, NULL, 'q'},
        {"unaligned", no_argument, NULL, 'u'},
        {"threads", required_argument, NULL, 't'},
        {"target-addr", required_argument, NULL, 'a'},
        {0,0,0,0}
    };

    int opt, opt_index;


    //set default UI type
    args->ui_type = UI_TERM;
    args->ptr_lookback = 0x400;
    args->target_addr = 0;
    args->levels = 5;
    args->aligned = true;

    //option processing while loop
    while((opt = getopt_long(argc, argv, "cnp:l:s:qut:a:", long_opts, &opt_index)) != -1
          && opt != 0) {

        //determine parsed argument
        switch (opt) {

            case 'c': //terminal UI
                args->ui_type = UI_TERM;
                break;

            case 'n': //ncurses UI
                args->ui_type = UI_NCURSES;
                break;

            case 'p': //pointer lookback
                args->ptr_lookback = 
                    (uintptr_t) process_int_argument(exception_str[0]);
                break;
            
            case 'l': //depth level
                args->levels = (unsigned int) process_int_argument(exception_str[1]);
                break;
            
            case 's': //extra memory regions to treat as static
                process_extra_static_regions(args, optarg, exception_str[2]);
                break;

            case 'q': //aligned pointer scan
                args->aligned = SCAN_ALIGNED;
                break;

            case 'u': //unaligned pointer scan
                args->aligned = SCAN_UNALIGNED;
                break;

            case 't': //number of threads
                args->num_threads = 
                    (unsigned int) process_int_argument(exception_str[3]);
                break;

            case 'a': //target address
                args->target_addr = (uintptr_t) process_int_argument(exception_str[4]); 
                break;

        } //end switch

    } //end opt while loop


    //assign target string and check for null
    if (argv[optind] == 0) {
        throw std::runtime_error(exception_str[5]);
    }
    args->target_str.assign(argv[optind]);

    return;
}
