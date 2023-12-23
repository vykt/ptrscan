#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cstring>
#include <cstdint>

#include "serialise.h"
#include "args.h"
#include "mem_tree.h"


//wrapper for fwrite() to get rid of failure testing
inline void record_value(void * value, size_t size, size_t nmemb, FILE * fs) {

    const char * exception_str[1] = {
        "serialise -> record_value: fwrite() wrote less/more than requested."
    };

    size_t write_last;

    write_last = fwrite(value, size, nmemb, fs);
    if (write_last != 1) {
        throw std::runtime_error(exception_str[0]);
    }
}


//add offset from previous node to current node, producing an offset chain
void serialise::recurse_get_next_offset(mem_node * m_node, serial_entry * s_entry,
                                        uintptr_t last_point) {

    //add offset for this node
    s_entry->offset_vector->push_back((uint32_t) m_node->node_addr - last_point);

    //recurse if this is not the root node
    if (m_node->parent_node != nullptr) {
        this->recurse_get_next_offset(m_node->parent_node, s_entry,
                                      m_node->point_addr);
    }

    return;
}


/*
 *  this function does not check children of static nodes (desirable behaviour)
 */

//recurse down the tree from root looking for leaves or static nodes
void serialise::recurse_node(args_struct * args, mem_node * m_node, 
                             unsigned int current_level) {

    serial_entry temp_s_entry;

    //check if node is static or is at the final level
    if (m_node->static_regions_index != -1 || current_level >= args->levels - 1) {
        
        //setup new serial entry
        memset(&temp_s_entry, 0, sizeof(temp_s_entry));
        temp_s_entry.static_regions_index = m_node->static_regions_index;
        temp_s_entry.offset_vector = new std::vector<uint32_t>;

        //recursively traverse to root, adding offsets in the process
        this->recurse_get_next_offset(m_node->parent_node, &temp_s_entry,
                                      m_node->point_addr);
    
    //else recurse for all child nodes
    } else {

        //for each child node
        for (std::list<mem_node>::iterator it = m_node->subnode_list.begin();
             it != m_node->subnode_list.end(); ++it) {

            //recurse down
            this->recurse_node(args, &(*it), current_level + 1);
            
        } //end for
    } //end if-else

    return;
}


/*
 *  NOTE! call either tree_to_results() OR read_disk_results() to build results vector
 */

//convert mem_tree to a serialised format that can be stored and printed.
void serialise::tree_to_results(args_struct * args, mem_tree * m_tree, 
                                proc_mem * p_mem) {

    char * name_substring;

    //for each static region
    for (unsigned int i = 0;
         i < (unsigned int) p_mem->static_regions_vector.size(); ++i) {

        //extract name from absolute path
        name_substring = strrchr(p_mem->static_regions_vector[i]->pathname, '/') + 1;
        if (name_substring == (char *) 1)
            name_substring = p_mem->static_regions_vector[i]->pathname;

        //add name to local static_regions_vector
        this->static_regions_vector.push_back(std::string(name_substring));

    } //end for

    //start recursive depth first traversal from root
    this->recurse_node(args, (mem_node *) m_tree->root_node, 0);

    return;
}


//write ptrchains to output file
void serialise::write_mem_ptrchains(args_struct * args, proc_mem * p_mem) {

    const char * exception_str[1] = {
        "serialise -> write_mem_ptrchains: fopen() failed.",
    };

    const byte delim_region = DELIM_REGION;
    const uint32_t delim_offset = DELIM_OFFSET; 

    FILE * fs;


    //get file stream for output file
    fs = fopen(args->output_file.c_str(), "w");
    if (fs == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }


    //for each static region definition
    for (unsigned int i = 0; 
         i < (unsigned int) p_mem->static_regions_vector.size(); ++i) {

        //record the static region name
        record_value((void *) this->static_regions_vector[i].c_str(), sizeof(char),
                     this->static_regions_vector[i].length(), fs);

        //write delimiter
        record_value((void *) &delim_region, sizeof(delim_region), 1, fs);

    } //end for
    
    //record additional delimiter to signify end of static region definitions
    record_value((void *) &delim_region, sizeof(delim_region), 1, fs);


    //for each offset set
    for (unsigned int i = 0; i < (unsigned int) this->ptrchains_vector.size(); ++i) {

        //record static region index
        record_value((void *) &this->ptrchains_vector[i].static_regions_index,
                     sizeof(this->ptrchains_vector[i].static_regions_index),
                     1,
                     fs);

        //for each offset in offset set
        for (unsigned int j = 0; 
             j < (unsigned int) this->ptrchains_vector[i].offset_vector->size(); ++j) {

            //record offset
            record_value((void *) &this->ptrchains_vector[i].offset_vector[j],
                         sizeof(this->ptrchains_vector[i].offset_vector[j]),
                         1,
                         fs);

        } //end inner for

        //write delimiter
        record_value((void *) &delim_offset, sizeof(delim_offset), 1, fs);

    } //end for

    //close file stream
    fclose(fs);

    return;
}


/*
 *  NOTE! call either tree_to_results() OR read_disk_results() to build results vector
 */

//read ptrchains from input file
void serialise::read_disk_ptrchains(args_struct * args) {

    const char * exception_str[1] = {
        "serialise -> read_disk_ptrchains: fopen() failed.",
    };

    const byte delim_region = DELIM_REGION;
    const uint32_t delim_offset = DELIM_OFFSET; 

    FILE * fs;

    
    //get file stream for input file
    fs = fopen(args->output_file.c_str(), "r");
    if (fs == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }

    /*
     * 1) get region names a character at a time until encountering 2 DELIMs
     *
     * 2) get offsets 4 bytes at a time, checking for DELIMs and EOF
     *
     */

}
