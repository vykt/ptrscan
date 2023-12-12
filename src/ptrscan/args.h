#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <vector>

#include <libpwu.h>

#include "ui_base.h"


#define UI_TERM 0
#define UI_NCURSES 1

#define SCAN_ALIGNED true
#define SCAN_UNALIGNED false

#define STRTYPE_INT 0
#define STRTYPE_LONG 1
#define STRTYPE_LLONG 2


/*
 *  In the future, consider allowing the user to specify a custom offset, probably
 *  as an exponent of 2.
 */

typedef struct {

    std::string target_str;
    byte ui_type;
    bool aligned;
    uintptr_t ptr_lookback;
    uintptr_t target_addr;
    unsigned int levels;
    unsigned int num_threads;
    std::vector<static_region> extra_region_vector;

} args_struct;

void process_args(int argc, char ** argv, args_struct * args);

#endif
