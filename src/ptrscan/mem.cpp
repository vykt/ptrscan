#include <vector>
#include <string>
#include <stdexcept>

#include <cstring>

#include <unistd.h>
#include <linux/limits.h>

#include <libcmore.h>
#include <liblain.h>

#include "mem.h"
#include "args.h"
#include "ui_base.h"


// --- PRIVATE METHODS

inline void mem::interpret_target(args_struct * args) {

    const char * exception_str[4] = {
        "mem -> interpret_target: failed to retrieve process name for given PID.",
        "mem -> interpret_target: failed to run name matches for PID.",
        "mem -> interpret_target: no matches of name to PID.",
        "mem -> interpret_target: multiple matches of name to PID."
    };

    int ret;
    char proc_name[NAME_MAX];
    cm_vector pid_vector;

    region temp_s_region;

	//if the target string contains only digits then interpret as PID
	if(args->target_str.find_first_not_of("0123456789") == std::string::npos) {
        
        //set pid
        this->pid = (pid_t) std::stoi(args->target_str);
        
        //get process name
        ret = ln_name_by_pid(this->pid, proc_name);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //save target comm
        args->target_comm = proc_name;
    
    //otherwise interpret as process name (comm)
    } else {

        //get PIDs for this comm
        ret = ln_pid_by_name((const char *) args->target_str.c_str(), &pid_vector);
        if (ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }
        if (pid_vector.len == 0) {
            throw std::runtime_error(exception_str[2]);
        }
        if (pid_vector.len > 1) {
            throw std::runtime_error(exception_str[3]);
        }

        //cleanup
        cm_del_vector(&pid_vector);
    
        //save target comm
        args->target_comm = args->target_str;

    }

    return;
}


inline void mem::open_session(args_struct * args) {

    const char * exception_str[1] = {
        "mem -> open_session: failed to open a liblain session."
    };

    int ret;

    ret = ln_open(&this->session, args->ln_iface, this->pid);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


inline void mem::get_map() {

    const char * exception_str[1] = {
        "mem -> read_map: failed to get a map of the target."
    };

    int ret;

    ln_new_vm_map(&this->map);

    ret = ln_update_map(&this->session, &this->map);
    if (ret) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


inline void mem::add_standard_vmas(args_struct * args) {

    const region stack_region = { "[stack]", 0, 0 };
    const region heap_region = {"[heap]", 0, 0};
    const region bss_region = { args->target_comm, 0, 0 };

    //add standard vmas to always scan
    args->exclusive_rw_regions.insert(args->exclusive_rw_regions.begin(), 
                                      stack_region);
    args->exclusive_rw_regions.insert(args->exclusive_rw_regions.begin(), 
                                      heap_region);
    args->exclusive_rw_regions.insert(args->exclusive_rw_regions.begin(), 
                                      bss_region);

    //add standard vmas to always treat as static
    args->extra_static_regions.insert(args->extra_static_regions.begin(), 
                                      stack_region);
    args->extra_static_regions.insert(args->extra_static_regions.begin(), 
                                      bss_region);

    return;
}


inline void mem::add_static_vma(args_struct * args, cm_list_node * vma_node) {

    int ret;
    
    region * iter_region;
    ln_vm_area * vma = LN_GET_NODE_AREA(vma_node);


    //for every static region
    for (unsigned int i = 0; i < args->extra_static_regions.size(); ++i) {

        iter_region = &(args->extra_static_regions)[i];

        //continue if basename is not a match | vma->basename can't be null
        ret = strncmp(vma->basename, iter_region->pathname.c_str(), NAME_MAX);
        if (ret) continue;

        //continue if asked to skip first entr{y,ies}
        iter_region->skipped += 1;
        if (iter_region->skipped <= iter_region->skip) continue;

        //all checks paseed, add to static vector
        this->static_regions.insert(this->static_regions.end(), vma_node);
        
        //impossible to match more than once, safe to break
        break;

    } //end for every static region

    return;
}


void mem::populate_regions(args_struct * args) {

    const char * exception_str[1] = {
        "mem -> populate_regions: map's vma list is empty."
    };

    //TODO find & add others
    const char * vma_blacklist[VMA_BLACKLIST_SIZE] = {
        "/dev",
        "/memfd",
        "/run"
    };

    int ret;
    bool exclusive_rw_found;

    cm_list_node * vma_node;
    ln_vm_area * vma;


    //setup iteration
    vma_node = this->map.vm_areas.head;
    if (vma_node == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }
    vma = LN_GET_NODE_AREA(vma_node);

    //for every vma
    for (int i = 0; i < (int) this->map.vm_areas.len; ++i) {

        exclusive_rw_found = false;

        //if this vma has a backing object
        if (vma->pathname != NULL) {

            //check if this is a blacklisted vma
            for (int i = 0; i < VMA_BLACKLIST_SIZE; ++i) {
                if (!strncmp(vma->pathname, 
                             vma_blacklist[i], strlen(vma_blacklist[i]))) continue;
            }

        } //end if vma has backing object

        //if vma has rw-/rwx permissions
        if (vma->access & (LN_ACCESS_READ | LN_ACCESS_WRITE)) continue;

        //if user specified an exclusive set of vmas to scan
        if (!(args->exclusive_rw_regions.empty())) {

            //check if this vma is in the exclusive set
            for (int i = 0; i < args->exclusive_rw_regions.size(); ++i) {
                exclusive_rw_found = true;
            }
            if (!exclusive_rw_found) continue;

        } //end if exclusive set applies

        //all checks passed, add to rw-/rwx vector
        this->rw_regions.insert(this->rw_regions.end(), vma_node);

        //check if this region should also be added as static
        add_static_vma(args, vma_node);
    
    } //end for

    return;
}


// --- PUBLIC METHODS

//constructor
void mem::init_mem(args_struct * args, ui_base * ui) {

    int ret;

    //get PID for target process
	interpret_target(args);

    //open a procfs or lainko session
    open_session(args);

    //read process maps
    get_map();

    //get a vector of every rw- memory region
    populate_regions(args);

    return;
}


inline std::vector<cm_list_node *> * mem::get_rw_regions() {
    return &this->rw_regions;
}


inline std::vector<cm_list_node *> * mem::get_static_regions() {
    return &this->static_regions;
}
