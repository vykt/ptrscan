#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <vector>

#include <libpwu.h>

#include "ui_base.h"


typedef struct {

    std::string target_str;
    byte ui_type;
    uintptr_t ptr_lookback;
    unsigned int levels;
    std::vector<static_region> extra_region_vector;

} args_struct;

void process_args(int argc, char ** argv, args_struct * args);

#endif
