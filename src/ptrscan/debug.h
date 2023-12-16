#ifndef DEBUG_H
#define DEBUG_H

#include "args.h"
#include "proc_mem.h"
#include "thread_ctrl.h"
#include "mem_tree.h"

void dump_structures_init(args_struct * args, proc_mem * p_mem);
void dump_structures_thread_level(thread_ctrl * t_ctrl, mem_tree * m_tree, int lvl);

#endif
