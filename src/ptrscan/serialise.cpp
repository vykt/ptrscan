#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cstring>
#include <cstdint>

#include <linux/limits.h>

#include <libcmore.h>
#include <liblain.h>

#include "serialise.h"
#include "args.h"
#include "mem_tree.h"


// --- INTERNAL

//handle fgetc() errors
static inline bool _handle_fgetc_err(int ret, FILE * fs, bool get_EOF) {

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
static inline uint32_t _get_stream_uint32_t(FILE * fs) {

    uint32_t ret;
    byte * ret_ptr;
    
    ret_ptr = (byte *) &ret;

    //for each byte of num
    for (unsigned int i = 0; i < (unsigned int) sizeof(ret); ++i) {

        *(ret_ptr+i) = (byte) fgetc(fs);
        handle_fgetc_err(*(&ret+i), fs, false);

    } //end for

    return ret;
}


//wrapper for fwrite() to get rid of failure testing
static inline void _record_value(void * value, size_t size, size_t nmemb, FILE * fs) {

    const char * exception_str[1] = {
        "serialise -> record_value: fwrite() wrote less/more than requested."
    };

    size_t write_last;

    write_last = fwrite(value, size, nmemb, fs);
    if (write_last != 1) {
        throw std::runtime_error(exception_str[0]);
    }
}


// --- PRIVATE METHODS

//add offset from previous node to current node, producing an offset chain
void serialise::recurse_get_next_offset(mem_node * m_node, serial_entry * s_entry,
                                        uintptr_t last_point) {

    //add offset for this node
    uint32_t offset;

    offset = (uint32_t) (m_node->get_addr() - last_point);

    s_entry->offset_vector->push_back(offset);

    //recurse if this is not the root node
    if (m_node->get_parent() != nullptr) {
        this->recurse_get_next_offset(m_node->get_parent(), s_entry,
                                      m_node->get_ptr_addr());
    }

    return;
}


//recurse down the tree from root looking for leaves or static nodes
void serialise::recurse_node(args_struct * args, mem * m, 
                             mem_node * m_node, unsigned int current_depth) {

    int ret;
    
    uint32_t offset;

    serial_entry temp_s_entry;
    std::list<mem_node> * children;

    ln_vm_area * vma;


    //if this node is not in a read & write region it is a false positive
    if (m_node->get_rw_regions_index() == -1) return;

    //check if node is static or recursion reached max depth
    if ((m_node->get_static_regions_index() != -1) 
        || (current_depth >= args->max_depth - 1)) {
        
        //setup new serial entry
        memset(&temp_s_entry, 0, sizeof(temp_s_entry));
        temp_s_entry.rw_regions_index = m_node->get_rw_regions_index();
        temp_s_entry.static_regions_index = m_node->get_static_regions_index();
        temp_s_entry.offset_vector = new std::vector<uint32_t>;
        
        //calculate first offset from the start of maps_obj to node's address
        vma = LN_GET_NODE_AREA(m_node->get_vma_node());
        offset = (uint32_t) ln_get_obj_offset(vma->obj_node_ptr, m_node->get_addr());

        //add the offset
        temp_s_entry.offset_vector->push_back(offset);

        //recursively traverse to root, adding offsets in the process
        this->recurse_get_next_offset(m_node->get_parent(), &temp_s_entry,
                                      m_node->get_ptr_addr());

        //add this serial entry to the ptrchains vector
        this->ptrchains_vector.push_back(temp_s_entry);

    //else recurse for all child nodes
    } else {

        children = m_node->get_children();

        //for each child node
        for (std::list<mem_node>::iterator it = children->begin();
             it != children->end(); ++it) {

            //recurse down
            this->recurse_node(args, m, &(*it), current_depth + 1);
            
        } //end for
    } //end if-else

    return;
}


// --- PUBLIC METHODS

//convert mem_tree to a serialised format that can be stored and printed.
void serialise::tree_to_results(args_struct * args, mem * m, mem_tree * m_tree) {

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
    regions_vectors[STATIC_REGIONS] = &this->static_region_str_vector;
    regions_vectors[RW_REGIONS] = &this->rw_region_str_vector;


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
                         (*regions_vectors[i])[j].length(), sizeof(char), fs);

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

        //record rw region index
        record_value((void *) &this->ptrchains_vector[i].rw_regions_index,
                     sizeof(this->ptrchains_vector[i].rw_regions_index),
                     1,
                     fs);

        //for each offset in offset set
        for (unsigned int j = 0; 
             j < (unsigned int) this->ptrchains_vector[i].offset_vector->size(); ++j) {

            //record offset
            record_value((void *) &(*this->ptrchains_vector[i].offset_vector)[j],
                         sizeof((*this->ptrchains_vector[i].offset_vector)[j]),
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
    uint32_t static_region_id_buf, rw_region_id_buf, offset_buf;
    serial_entry temp_s_entry;

    int ret;
    FILE * fs;
    std::vector<std::string> * regions_vectors[2];


    //setup regions_vector array
    regions_vectors[RW_REGIONS] = &this->rw_region_str_vector;
    regions_vectors[STATIC_REGIONS] = &this->static_region_str_vector;

    
    //get file stream for input file
    fs = fopen(args->input_file.c_str(), "r");
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

            //read the next region entry
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
        static_region_id_buf = get_stream_uint32_t(fs);
        temp_s_entry.static_regions_index = static_region_id_buf;

        //get the rw region id
        rw_region_id_buf = get_stream_uint32_t(fs);
        temp_s_entry.rw_regions_index = rw_region_id_buf;

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
