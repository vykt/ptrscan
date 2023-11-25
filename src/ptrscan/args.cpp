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
        lookahead_comma = strrchr(next_region_str, ',');
        if (lookahead_comma == NULL) {
            throw std::runtime_error(exception_str[0]);
        }

        lookahead_slash = strrchr(next_region_str, ':');

        //check if next_region_str doesn't have a comma 
        if (lookahead_slash != NULL && lookahead_comma > lookahead_slash) {
            throw std::runtime_error(exception_str[0]);
        }

        //build temp_region
        temp_region.pathname.assign(next_region_str, 
                                    lookahead_comma - next_region_str - 1);

        temp_region.skip = atoi(lookahead_comma);
        temp_region.skipped = 0;

        //add temp region
        args->extra_region_vector.insert(args->extra_region_vector.end(), temp_region);

    } while((next_region_str = strrchr(next_region_str, ':')) != nullptr);
}



void process_args(int argc, char ** argv, args_struct * args) {

    const char * exception_str[3] = {
        "process_args: no argument provided for -p --ptr-lookback",
        "process_args: no argument provided for -l --levels",
        "process_args: no argument provided for -s --extra-static-regions"
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
                if (optarg == nullptr) {
                    std::cerr << "use: -p <limit:uint> --ptr-lookback=<limit:uint>"
                              << std::endl;
                    throw std::runtime_error(exception_str[0]);
                }
                args->ptr_lookback = (uintptr_t) atol(optarg);
                break;
            
            case 'l': //depth level
                if (optarg == nullptr) {
                    std::cerr << "use: -l <depth:uint> --levels=<depth:uint>"
                              << std::endl;
                    throw std::runtime_error(exception_str[0]);
                }
                break;
            
            case 's': //extra memory regions to treat as static
                if (optarg == nullptr) {
                    std::cerr << "use: -s <region> --extra-static-regions=<region>"
                              << std::endl;
                    throw std::runtime_error(exception_str[0]);
                }
                //process each additional static region
                process_extra_static_regions(args, optarg);
                break;

        } //end switch

    } //end opt while loop

    //assign target string
    args->target_str.assign(argv[optind]);    

    return;
}
