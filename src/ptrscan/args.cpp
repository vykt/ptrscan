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



void process_extra_static_regions(args_struct * args, char * regions) {

    const char * exception_str[1] {
        "process_extra_static_regions: invalid static region format."
    };

    char * next_region_str;
    char * lookahead_comma;
    char * lookahead_slash;

    static_region temp_region;

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



void process_args(int argc, char ** argv, args_struct * args) {

    const char * exception_str[4] = {
        "process_args: use: -p <lookback:int> --ptr-lookback=<lookback:int>",
        "process_args: use: -l <depth> --levels=<depth>",
        "process_args: use: -s <name:str>,<skip:int> \
--extra-static-regions=<name:str>,<skip:int>:<...>",
        "process_args: use: ptrscan [flags] <target_name>"
    };

    //defined cmdline options
    struct option long_opts[] = {
        {"ui-term", no_argument, nullptr, 't'},
        {"ui-ncurses", no_argument, nullptr, 'n'},
        {"ptr-lookback", required_argument, nullptr, 'p'},
        {"levels", required_argument, nullptr, 'l'},
        {"extra-static-regions", required_argument, NULL, 's'},
        {0,0,0,0}
    };

    int opt, opt_index;


    //set default UI type
    args->ui_type = UI_TERM;
    args->ptr_lookback = 1000;
    args->levels = 5;

    //TODO check all of these type conversions are correct, they likely arent TODO

    //option processing while loop
    while((opt = getopt_long(argc, argv, "tnp:l:s:", long_opts, &opt_index)) != -1) {

        //determine parsed argument
        switch (opt) {

            case 't': //terminal UI
                args->ui_type = UI_TERM;
                break;

            case 'n': //ncurses UI
                args->ui_type = UI_NCURSES;
                break;

            case 'p': //pointer lookback
                //check for null
                if (optarg == nullptr) {
                    throw std::runtime_error(exception_str[0]);
                }
                //try to convert to address int
                try {
                    args->ptr_lookback = (uintptr_t) std::stoll(optarg);
                } catch (std::exception &e) {
                    throw std::runtime_error(exception_str[0]);
                }
                break;
            
            case 'l': //depth level
                //check for null
                if (optarg == nullptr) {
                    throw std::runtime_error(exception_str[1]);
                }
                //try to convert to unsigned int
                try {
                    args->levels = (unsigned int) std::stoi(optarg);
                } catch(std::exception& e) {
                    throw std::runtime_error(exception_str[1]);
                }
                break;
            
            case 's': //extra memory regions to treat as static
                if (optarg == nullptr) {
                    throw std::runtime_error(exception_str[2]);
                }
                //process each additional static region
                process_extra_static_regions(args, optarg);
                break;

        } //end switch

    } //end opt while loop

    //assign target string and check for null
    args->target_str.assign(argv[optind]);
    if (args->target_str == "") {
        throw std::runtime_error(exception_str[3]);
    }

    return;
}
