#include <string>
#include <stdexcept>

#include <cstdint>

#include <unistd.h>
#include <errno.h>

#include <libpwu.h>

#include "verify.h"
#include "args.h"
#include "proc_mem.h"
#include "serialise.h"




//verify individual pointer chain
int verify_chain(args_struct * args, proc_mem * p_mem, ui_base * ui, 
                 serial_entry * s_entry, maps_obj * matched_obj) {

    const char * exception_str[3] = {
        "verify_chain: vector_get_ref() failed on matched_obj.",
        "verify_chain: can't lseek() to remote address."
        "verify_chain: non EIO errno when dereferencing address."
    };

    int ret;
    ssize_t rd_bytes;
    uintptr_t remote_addr;

    maps_entry * first_m_entry;


    //get starting address for object
    ret = vector_get_ref(&matched_obj->entry_vector, 0, (byte **) &first_m_entry);
    if (ret == -1) {
        throw std::runtime_error(exception_str[0]);
    }
    remote_addr = (uintptr_t) first_m_entry->start_addr;


    //for every offset
    for (unsigned int i = 0; i < (unsigned int) s_entry->offset_vector->size(); ++i) {

        //get next address to dereference
        remote_addr += (*s_entry->offset_vector)[i];
        
        //seek to address
        ret = (int) lseek(p_mem->mem_fd, remote_addr, SEEK_SET);
        if (ret == -1) {
            throw std::runtime_error(exception_str[1]);
        }

        //dereference address
        rd_bytes = read(p_mem->mem_fd, &remote_addr, sizeof(remote_addr));
        if (rd_bytes == -1) {
            //if trying to read outside VM regions (invalid chain)
            if (errno == EIO) {
                return -1;
            //otherwise error occured
            } else {
                throw std::runtime_error(exception_str[2]);
            }
        }

    } //end for 

    
    //compare final address with target address
    if (remote_addr == args->target_addr) {
        return 0;
    } else {
        return -1;
    }
}


//verify results read from disk
void verify(args_struct * args, proc_mem * p_mem, ui_base * ui, serialise * ser) {

    const char * exception_str[1] = {
        "verify: "
    };

    int ret;

    maps_entry * temp_m_entry;
    maps_obj * temp_m_obj;

    serial_entry * temp_s_entry;
    std::string * tse_basename;


    //for every serial entry
    for (unsigned int i = 0; i < (unsigned int) ser->ptrchains_vector.size(); ++i) {

        //get next serial entry
        temp_s_entry = &ser->ptrchains_vector[i];
        tse_basename = &ser->rw_region_str_vector[temp_s_entry->rw_regions_index];

        //get an object with matching basename
        ret = get_region_by_basename((char *) tse_basename->c_str(), 0, &p_mem->m_data,
                                     &temp_m_entry, &temp_m_obj);
        
        //if match not found, delete chain
        if (ret == -1) {
            //TODO report deletion of entry
            ser->ptrchains_vector.erase(ser->ptrchains_vector.begin() + i);
            --i;
        }

        //check if pointer chain arrives at the correct address
        ret = verify_chain(args, p_mem, ui, temp_s_entry, temp_m_obj);
        if (ret == -1) {
            //incorrect chain, therefore delete entry
            ser->ptrchains_vector.erase(ser->ptrchains_vector.begin() + i);
        }

    } //end for
}
