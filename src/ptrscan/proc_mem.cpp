#include <vector>
#include <string>
#include <stdexcept>

#include <cstring>

#include <unistd.h>
#include <linux/limits.h>

#include <libpwu.h>

#include "proc_mem.h"
#include "args.h"
#include "ui_base.h"


void proc_mem::interpret_target(args_struct * args, ui_base * ui) {

    const char * exception_str[4] = {
        "proc_mem -> interpret_target: failed to retrieve process name for given PID.",
        "proc_mem -> interpret_target: failed to initialise new_name_pid struct.",
        "proc_mem -> interpret_target: failed to run name matches for PID.",
        "proc_mem -> interpret_target: no matches of name to PID."
    };

    int ret;
    char proc_name[NAME_MAX] = {};

    region temp_s_region;

	//if the target string contains only digits then interpret as PID
	if(args->target_str.find_first_not_of("0123456789") == std::string::npos) {
        
        //set pid
        this->pid = (pid_t) std::stoi(args->target_str);
        
        //get process name
        ret = name_by_pid(this->pid, proc_name);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //set target_str to proc_name to correctly define static regions later
        args->target_str = proc_name;

        //TODO remove, shouldn't be necessary (test first)
        //create new region to treat as static using the process name
        /*temp_s_region.pathname = proc_name;
        temp_s_region.pathname.erase(temp_s_region.pathname.size() - 1);
        temp_s_region.skip = 0;
        temp_s_region.skipped = 0;

        //add static region
        args->extra_static_vector.push_back(temp_s_region);
        */  
        return;
    }

    //otherwise interpret as process name

    //initialise name_pid structure
    name_pid n_pid;
    ret = new_name_pid(&n_pid, (char *) args->target_str.c_str());
    if (ret) {
        throw std::runtime_error(exception_str[1]);
    }

    //fetch PIDs for name & deal with result
    ret = pid_by_name(&n_pid, &this->pid);
    switch (ret) {
        case -1:
            throw std::runtime_error(exception_str[2]);
            break;
        case 0:
            throw std::runtime_error(exception_str[3]);
            break;
        case 1:
            break;
        default:
            this->pid = ui->clarify_pid(&n_pid);
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


inline void proc_mem::add_static(args_struct * args, maps_entry * m_entry) {

    int ret;
    region * temp_region;

    const char * name_substring = strrchr((const char *) m_entry->pathname, '/') + 1;
    if (name_substring == (char *) 1) name_substring = m_entry->pathname;

    //for every static region
    for (unsigned int i = 0; i < args->extra_static_vector.size(); ++i) {

        temp_region = &(args->extra_static_vector)[i];

        //continue if pathname doesn't match
        ret = strncmp(name_substring, temp_region->pathname.c_str(), NAME_MAX);
        if (ret) continue;

        //continue if asked to skip first entr{y,ies}
        temp_region->skipped += 1;
        if (temp_region->skipped <= temp_region->skip) continue;

        //if reached here, its a match
        this->static_regions_vector.insert(this->static_regions_vector.end(), m_entry);
        args->extra_static_vector.erase(args->extra_static_vector.begin() + i);
        break; //should never match more than one entry

    } //end for every static region

    return;
}


void proc_mem::populate_regions(args_struct * args) {

    const char * exception_str[1] = {
        "proc_mem -> populate_rw_regions: vector get reference error."
    };

    //TODO add others
    const char * segment_blacklist[SEG_BLACKLIST_SIZE] = {
        "/dev",
        "/memfd"
    };

    const region stack_region = { "[stack]", 0, 0 };
    const region bss_region = { args->target_str, 0, 0 };

    int ret;
    bool extra_rw_found;
    maps_entry * m_entry;


    //add standard rw regions to rw_regions_vector
    args->extra_rw_vector.insert(args->extra_rw_vector.begin(), stack_region);
    args->extra_rw_vector.insert(args->extra_rw_vector.begin(), bss_region);

    //add standard static regions to static_regions_vector
    args->extra_static_vector.insert(args->extra_static_vector.begin(), stack_region);
    args->extra_static_vector.insert(args->extra_static_vector.begin(), bss_region);

    //for every memory region with distinct access permissions
    for (int i = 0; i < (int) this->m_data.entry_vector.length; ++i) {

        extra_rw_found = false;

        ret = vector_get_ref(&this->m_data.entry_vector, (unsigned long) i,
                             (byte **) &m_entry);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //skip blacklisted segments
        for (int i = 0; i < SEG_BLACKLIST_SIZE; ++i) {
            if (!strncmp(m_entry->pathname, segment_blacklist[i], strlen(segment_blacklist[i]))) continue;
        }

        //test for read & write permissions
        if (!((m_entry->perms & 0x01) && (m_entry->perms & 0x02))) {
            continue;
        }

        //check if exhaustive rw region vector is in use
        if (!(args->extra_rw_vector.empty())) {
            
            //for all regions in exhaustive rw region vector
            for (int i = 0; i < args->extra_rw_vector.size(); ++i) {
                if (args->extra_rw_vector[i].pathname == m_entry->basename) {
                    extra_rw_found = true;
                    break;
                }
            } //end for

            if (!extra_rw_found) continue;
        } //end exhaustive rw vector check


        //passed all checks, add to rw- vector
        this->rw_regions_vector.insert(this->rw_regions_vector.end(), m_entry);

        //add static region
        add_static(args, m_entry);
    
    } //end for

    return;
}


/*
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
void proc_mem::init_proc_mem(args_struct * args, ui_base * ui) {

    const char * exception_str[1] = {
        "proc_mem -> constructor: failed to open handles on proc maps and mem."
    };

    int ret;

    //get PID for target process
	interpret_target(args, ui);

    //open file handles
    ret = open_memory(this->pid, &this->maps_stream, &this->mem_fd);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    //read proc maps
    maps_init(&this->m_data);

    //get a vector of every rw- memory region
    populate_regions(args);

    return;
}
