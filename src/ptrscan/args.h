#ifndef ARGS_H
#define ARGS_H

#include "ui_base.h"


int process_extra_static_regions(args_struct * args, char * regions);
int process_args(int argc, char ** argv, args_struct * args);

#endif
