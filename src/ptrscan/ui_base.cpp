#include <string>
#include <stdexcept>

#include <cstring>

#include <libpwu.h>

#include "proc_mem.h"


//match maps_obj structure with a given basename string
int match_maps_obj(std::string basename, void * proc_mem_ptr, 
                    maps_obj ** matched_m_obj) {

    const char * exception_str[1] = {
        "match_maps_obj: failed to get a reference for maps_obj from libpwu vector."
    };

    int ret;
    maps_obj * temp_m_obj;

    //typecast arg
    proc_mem * real_p_mem = (proc_mem *) proc_mem_ptr;


    //for each maps_obj
    for (unsigned long i = 0; i < real_p_mem->m_data.obj_vector.length; ++i) {

        //get next maps_obj
        ret = vector_get_ref(&real_p_mem->m_data.obj_vector, i, (byte **) &temp_m_obj);
        if (ret == -1) {
            throw std::runtime_error(exception_str[0]);
        }

        //see if basename matches request
        ret = strcmp(basename.c_str(), temp_m_obj->basename);
        if (!ret) {
            *matched_m_obj = temp_m_obj;
            return 0;
        }

    } //end for

    return -1;
}
