#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cstring>
#include <cstdint>

#include <linux/limits.h>

#include <libpwu.h>

#include "serialise.h"
#include "args.h"
#include "mem_tree.h"


//handle fgetc() errors
inline bool handle_fgetc_err(int ret, FILE * fs, bool get_EOF) {

    const char * exception_str[2] = {
        "serialise -> read_disk_ptrchains: error reading file.",
        "serialise -> get_stream_uint32_t: unexpected end of file."
    };

    //check EOF
    if (ret == EOF) {
        //if error occured, throw error exception
        if (ferror(fs)) {
            throw std::runtime_error(exception_str[0]);
        //else throw unexpected file end exception
        } else {
            //return EOF to caller if requested
            if (get_EOF) {
                return 1;
            } else {
                throw std::runtime_error(exception_str[1]);
            }
        }
    } //end if
    
    return 0;
}


//get uint32_t from file stream
inline uint32_t get_stream_uint32_t(FILE * fs) {

    uint32_t ret;

    //for each byte of num
    for (unsigned int i = 0; i < (unsigned int) sizeof(ret); ++i) {

        *(&ret+i) = fgetc(fs);
        handle_fgetc_err(*(&ret+i), fs, false);

    } //end for

    return ret;
}



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
void serialise::recurse_node(args_struct * args, mem_node * m_node, proc_mem * p_mem, 
                             unsigned int current_level) {

    const char * exception_str[1] = {
        "serialise -> recurse_node: vector_get_ref() failed when getting first offset"
    };


    int ret;
    uint32_t first_offset;
    serial_entry temp_s_entry;

    unsigned long m_obj_index;

    maps_obj * m_obj;
    maps_entry * m_obj_first_m_entry;


    //if this node is not in a read & write region it is a false positive
    if (m_node->rw_regions_index == -1) return;

    //check if node is static or is at the final level
    if (m_node->static_regions_index != -1 || current_level >= args->levels - 1) {
        
        //setup new serial entry
        memset(&temp_s_entry, 0, sizeof(temp_s_entry));
        temp_s_entry.rw_regions_index = m_node->rw_regions_index;
        temp_s_entry.static_regions_index = m_node->static_regions_index;
        temp_s_entry.offset_vector = new std::vector<uint32_t>;

        //calculate first offset from the start of maps_obj to node's address
        
        //1. get maps_obj index
        m_obj_index = 
            p_mem->rw_regions_vector[temp_s_entry.rw_regions_index]->obj_vector_index;
        
        //2. get maps_obj pointer
        ret = vector_get_ref(&p_mem->m_data.obj_vector, m_obj_index, 
                             (byte **) &m_obj);
        if (ret == -1) throw std::runtime_error(exception_str[0]);

        //3. get the first entry of the maps_obj
        ret = vector_get(&m_obj->entry_vector, 0, (byte *) &m_obj_first_m_entry);
        if (ret == -1) throw std::runtime_error(exception_str[0]);

        //4. calculate the offset
        first_offset = (uint32_t) (m_node->node_addr 
                                   - (uintptr_t) m_obj_first_m_entry->start_addr);

        //add the offset
        temp_s_entry.offset_vector->push_back(first_offset);

        //recursively traverse to root, adding offsets in the process
        this->recurse_get_next_offset(m_node->parent_node, &temp_s_entry,
                                      m_node->point_addr);

        //add this serial entry to the ptrchains vector
        this->ptrchains_vector.push_back(temp_s_entry);


    //else recurse for all child nodes
    } else {

        //for each child node
        for (std::list<mem_node>::iterator it = m_node->subnode_list.begin();
             it != m_node->subnode_list.end(); ++it) {

            //recurse down
            this->recurse_node(args, &(*it), p_mem, current_level + 1);
            
        } //end for
    } //end if-else

    return;
}


/*
 *  NOTE! call either tree_to_results() OR read_disk_results() to build results vector
 */

//convert mem_tree to a serialised format that can be stored and printed.
void serialise::tree_to_results(args_struct * args, proc_mem * p_mem, 
                                mem_tree * m_tree) {

    char * name_substring;

    /*
     *  This process could be more efficient, its a refactor after a substantial fuckup
     */

    //for each rw region, extract strings
    for (unsigned int i = 0;
         i < (unsigned int) p_mem->rw_regions_vector.size(); ++i) {

        //extract name from absolute path
        name_substring = strrchr(p_mem->rw_regions_vector[i]->pathname, '/') + 1;
        if (name_substring == (char *) 1)
            name_substring = p_mem->rw_regions_vector[i]->pathname;

        //add name to local static_region_str_vector
        this->rw_region_str_vector.push_back(std::string(name_substring));

    } //end for


    //for each static region, extract strings
    for (unsigned int i = 0;
         i < (unsigned int) p_mem->static_regions_vector.size(); ++i) {

        //extract name from absolute path
        name_substring = strrchr(p_mem->static_regions_vector[i]->pathname, '/') + 1;
        if (name_substring == (char *) 1)
            name_substring = p_mem->static_regions_vector[i]->pathname;

        //add name to local static_region_str_vector
        this->static_region_str_vector.push_back(std::string(name_substring));

    } //end for


    //start recursive depth first traversal from root
    this->recurse_node(args, (mem_node *) m_tree->root_node, p_mem, 0);

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
    std::vector<std::string> * regions_vectors[2];


    //setup regions_vector array
    regions_vectors[RW_REGIONS] = &this->rw_region_str_vector;
    regions_vectors[STATIC_REGIONS] = &this->static_region_str_vector;


    //get file stream for output file
    fs = fopen(args->output_file.c_str(), "w");
    if (fs == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }


    //for each type of region
    for (int i = 0; i < REGIONS_TYPE_NUM; ++i) {

        //for each static region definition
        for (unsigned int j = 0; j < (unsigned int) regions_vectors[i]->size(); ++j) {

            //record the static region name
            record_value((void *) (*regions_vectors[i])[j].c_str(), 
                         sizeof(char), (*regions_vectors[i])[j].length(), fs);

            //write delimiter
            record_value((void *) &delim_region, sizeof(delim_region), 1, fs);

        } //end inner for
        
        //record additional delimiter to signify end of static region definitions
        record_value((void *) &delim_region, sizeof(delim_region), 1, fs);

    } //end for


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

    const char * exception_str[2] = {
        "serialise -> read_disk_ptrchains: fopen() failed.",
        "serialise -> read_disk_ptrchains: ungetc() failed.",
    };

    const byte delim_region = DELIM_REGION;
    const uint32_t delim_offset = DELIM_OFFSET; 

    char region_def_buf[NAME_MAX];
    uint32_t region_id_buf, offset_buf;
    serial_entry temp_s_entry;

    int ret;
    FILE * fs;
    std::vector<std::string> * regions_vectors[2];


    //setup regions_vector array
    regions_vectors[RW_REGIONS] = &this->rw_region_str_vector;
    regions_vectors[STATIC_REGIONS] = &this->static_region_str_vector;

    
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

    //for each type of region
    for (int i = 0; i < REGIONS_TYPE_NUM; ++i) {

        //read through static region definitions
        while ((ret = fgetc(fs)) != delim_region) {

            //handle eval errors
            handle_fgetc_err(ret, fs, false);

            //push char back on stream
            ret = ungetc(ret, fs);
            if (ret == EOF) {
                throw std::runtime_error(exception_str[1]);
            }

            //clear buffer
            memset(region_def_buf, 0, NAME_MAX);

            //read the next static region entry
            for (unsigned int j = 0; j < NAME_MAX+1; ++j) {

                //get next character
                region_def_buf[j] = (char) fgetc(fs);
                handle_fgetc_err(region_def_buf[j], fs, false);
               
                //check if character is a delimiter
                if (region_def_buf[j] == delim_region) {
                    region_def_buf[j] = '\0';
                    break;
                }

            } //end inner for

            //add region to appropriate region vector
            (*regions_vectors[i]).push_back(region_def_buf);

        } //end while
    } //end for


    //read through offset sets until encountering a valid EOF
    while (!handle_fgetc_err(ret = fgetc(fs), fs, true)) {
        
        //push char back on stream
        ret = ungetc(ret, fs);
        if (ret == EOF) {
            throw std::runtime_error(exception_str[1]);
        }

        //setup new temporary serial entry
        memset(&temp_s_entry, 0, sizeof(temp_s_entry));
        temp_s_entry.offset_vector = new std::vector<uint32_t>;

        //get the static region id
        region_id_buf = get_stream_uint32_t(fs);
        temp_s_entry.static_regions_index = region_id_buf;

        //while there are offsets to read
        while((offset_buf = get_stream_uint32_t(fs)) != delim_offset) {
           
            //push back offset to offset vector for this offset set
            temp_s_entry.offset_vector->push_back(offset_buf);

        } //end inner while

        //add temporary serial entry to pointer chain vector
        this->ptrchains_vector.push_back(temp_s_entry);

    } //end while


    //close file stream
    fclose(fs);

    return;
}
