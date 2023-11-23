#include <iostream>

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <unistd.h>
#include <getopt.h>

#include <libpwu.h>

#include "ui_base.h"



int process_extra_static_regions(args_struct * args, char * regions) {

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
        if (lookahead_comma == NULL) return -1; //incorrect format

        lookahead_slash = strrchr(next_region_str, ':');

        //check if next_region_str doesn't have a comma 
        if (lookahead_slash != NULL && lookahead_comma > lookahead_slash) {
            return -1; //incorrect format
        }

        //build temp_region
        temp_region.pathname.assign(next_region_str, 
                                    lookahead_comma - next_region_str - 1);

        temp_region.skip = atoi(lookahead_comma);
        temp_region.skipped = 0;

        //add temp region
        args->extra_region_vector.insert(args->extra_region_vector.end(), temp_region);

    } while((next_region_str = strrchr(next_region_str, ':')) != nullptr);
    
    return 0;
}



int process_args(int argc, char ** argv, args_struct * args) {

    int ret, opt, opt_index;

    //defined options
    struct option long_opts[] = {
        {"ui-term", no_argument, nullptr, 't'},
        {"ui-ncurses", no_argument, nullptr, 'n'},
        {"ptr-lookback", required_argument, nullptr, 'p'},
        {"levels", required_argument, nullptr, 'l'},
        {"extra-static-regions", required_argument, NULL, 's'},
        {0,0,0,0}
    };

    //set default UI type
    args->ui_type = UI_TERM;

    //TODO check all of these type conversions are correct, they likely arent TODO

    //option processing while loop
    while((opt = getopt_long(argc, argv, "tnp:l:s:", long_opts, &opt_index)) != -1) {

        //determine parsed argument
        switch (opt) {

            case 't':
                args->ui_type = UI_TERM;
                break;

            case 'n':
                args->ui_type = UI_NCURSES;
                break;

            case 'p':
                if (optarg == nullptr) {
                    std::cerr << "use: -p <limit:uint> --ptr-lookback=<limit:uint>"
                              << std::endl;
                    return -1;
                }
                args->ptr_lookback = (uintptr_t) atol(optarg);
                break;
            
            case 'l':
                if (optarg == nullptr) {
                    std::cerr << "use: -l <depth:uint> --levels=<depth:uint>"
                              << std::endl;
                    return -1;
                }
                break;
            
            case 's':
                if (optarg == nullptr) {
                    std::cerr << "use: -s <region> --extra-static-regions=<region>"
                              << std::endl;
                    return -1;
                }
                ret = process_extra_static_regions(args, optarg);
                break;

        } //end switch

    } //end opt while loop

    return -1;
}
