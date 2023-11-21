#include <vector>
#include <string>

#include <libpwu.h>

#include "proc_mem.h"
#include "ui.h"


/*
 *  TODO
 *
 *  1) convert target_str to a PID, then open maps and mem for PID
 *
 *  2) libpwu init, fill this->m_data
 *
 *  3) populate rw_regions_vec
 *
 *  4) populate static_regions_vec with .bss and [stack]
 *
 *  4) process esr_vec and add any entries to static_regions_vec
 *
 */
proc_mem::proc_mem(std::string target_str, byte flags,
                   std::vector<extra_static_region> * esr_vec) {

    

}
