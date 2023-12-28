#ifndef VERIFY_H
#define VERIFY_H

#include "args.h"
#include "proc_mem.h"
#include "serialise.h"


/*
 *  verifying process separated from main serialise class due to lack of 
 *  direct relation.
 */

//verify results
void verify(args_struct * args, proc_mem * p_mem, ui_base * ui, serialise * ser);

#endif
