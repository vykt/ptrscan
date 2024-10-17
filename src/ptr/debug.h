#ifndef DEBUG_H
#define DEBUG_H

#include "args.h"
#include "mem.h"
#include "mem_tree.h"
#include "thread_ctrl.h"
#include "serialiser.h"


void dump_args(const args_struct * args);
void dump_mem(const mem * m);
void dump_mem_tree(const mem_tree * m_tree, const args_struct * args);
void dump_threads(const thread_ctrl * t_ctrl);
void dump_serialiser(const serialiser * s);


#endif
