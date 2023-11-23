#include <vector>
#include <string>
#include <stdexcept>

#include <cstring>

#include <unistd.h>
#include <linux/limits.h>

#include <libpwu.h>

#include "proc_mem.h"
#include "ui_base.h"




void proc_mem::fetch_pid(std::string target_str) {

    const char * exception_str[3] = {
        "proc_mem -> fetch_pid: failed to initialise new_name_pid struct.",
        "proc_mem -> fetch_pid: failed to run name matches for PID.",
        "proc_mem -> fetch_pid: no matches of name to PID."
    };

    int ret;
    pid_t pid;

    //initialise name_pid structure
    name_pid n_pid;
    ret = new_name_pid(&n_pid, (char *) target_str.c_str());
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //fetch PIDs for name & deal with result
    ret = pid_by_name(&n_pid, &this->pid);
    switch (ret) {
        case -1:
            throw std::runtime_error(exception_str[1]);
            break;
        case 0:
            throw std::runtime_error(exception_str[2]);
            break;
        case 1:
            break;
        default:
            //TODO CALL ui_base::clarify_pid() to get single pid from list of multiple
            break;
    }

    ret = del_name_pid(&n_pid);
    //not worth exception on fail

    return;
}


void proc_mem::maps_init(maps_data * m_data) {

    const char * exception_str[2] = {
        "proc_mem -> libpwu_init: failed to initialise maps_data struct.",
        "proc_mem -> libpwu_init: failed to populate maps_data struct."
    };

    int ret;

    //initialise maps_data structure
    ret = new_maps_data(m_data);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //read maps file and populate maps_data
    ret = read_maps(m_data, this->maps_stream);
    if (ret) {
        throw std::runtime_error(exception_str[1]);
    }
    
    return;
}


inline void proc_mem::add_static(std::vector<static_region> * static_vector,
                                 maps_entry * m_entry) {

    int ret;
    static_region * temp_region;

    const char * name_substring = strrchr((const char *) m_entry->pathname, '/');

    //for every static region
    for (int i = 0; i < static_vector->size(); ++i) {

        temp_region = &(*static_vector)[i];

        //continue if pathname doesn't match
        ret = strncmp(name_substring, temp_region->pathname.c_str(), NAME_MAX);
        if (ret) continue;

        //continue if asked to skip first entr{y,ies}
        temp_region->skipped += 1;
        if (temp_region->skipped <= temp_region->skip) continue;

        //if reached here, its a match
        this->static_regions_vector.insert(this->static_regions_vector.end(), m_entry);
        static_vector->erase(static_vector->begin() + i);
        break; //should never match more than one entry

    } //end for every static region

    return;
}


void proc_mem::populate_regions(std::vector<static_region> * extra_static_vector,
                                   std::string target_str) {

    const char * exception_str[1] = {
        "proc_mem -> populate_rw_regions: vector get reference error."
    };

    const static_region stack_region = { "[stack]", 0, 0 };
    const static_region bss_region = { target_str, 0, 0 };

    int ret;
    maps_entry * m_entry;


    //add standard static regions to static_regions_vector
    extra_static_vector->insert(extra_static_vector->begin(), stack_region);
    extra_static_vector->insert(extra_static_vector->begin(), bss_region);

    //for every memory region with distinct access permissions
    for (int i = 0; i < (int) this->m_data.entry_vector.length; ++i) {

        ret = vector_get_ref(&this->m_data.entry_vector, (unsigned long) i,
                             (byte **) &m_entry);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //test for read & write permissions
        if ((m_entry->perms & 0x02) && (m_entry->perms & 0x04)) {
            this->rw_regions_vector.insert(this->rw_regions_vector.end(), m_entry);
        } else {
            continue;
        }

        //add static region
        add_static(extra_static_vector, m_entry);
    
    } //end for

    return;
}


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
                   std::vector<static_region> * extra_static_vector) {

    const char * exception_str[1] = {
        "proc_mem -> constructor: failed to open handles on proc maps and mem."
    };

    int ret;
    
    //get PID for target process
    fetch_pid(target_str);

    //open file handles
    ret = open_memory(this->pid, &this->maps_stream, &this->mem_fd);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //read proc maps
    maps_init(&this->m_data);

    //TODO get the extra static vector from ui

    //get a vector of every rw- memory region
    populate_regions(/*STATIC REGION VECTOR TODO*/, target_str);

}
