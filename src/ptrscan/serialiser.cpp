#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cstring>
#include <cstdint>

#include <linux/limits.h>

#include <libcmore.h>
#include <liblain.h>

#include "serialiser.h"
#include "args.h"
#include "mem_tree.h"


// --- INTERNAL

//wrapper for fwrite()
static inline void _record_value(cm_byte * value, 
                                 size_t size, size_t nmemb, FILE * fs) {

    const char * exception_str[1] = {
        "serialise -> record_value: fwrite() request failed."
    };

    size_t write_last;

    write_last = fwrite(value, size, nmemb, fs);
    if (write_last != 1) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


//wrapper for fread
static inline void _read_value(cm_byte * value, 
                               size_t size, size_t nmemb, FILE * fs) {

    const char * exception_str[1] = {
        "serialise -> record_value: fread() request failed."
    };

    size_t read_last;

    read_last = fread(value, size, nmemb, fs);
    if (read_last != 1) {
        throw std::runtime_error(exception_str[0]);
    }

    return;
}


//wrapper for fgetc()
static inline char _read_char(FILE * fs, bool expect_EOF) {

    const char * exception_str[2] = {
        "serialise -> read_region_definitions: error reading .pscan file.",
        "serialise -> read_region_definitions: unexpected end of .pscan file."
    };

    int ret;

    ret = fgetc(fs);

    //check EOF
    if (ret == EOF) {
        //if error occured, throw error exception
        if (ferror(fs)) {
            throw std::runtime_error(exception_str[0]);
        //else throw unexpected file end exception
        } else {
            //return EOT to caller if requested
            if (expect_EOF) {
                return EOT; //EOT
            } else {
                throw std::runtime_error(exception_str[1]);
            }
        }
    } //end if
    
    return (char) ret;
}


//read region name from savefile stream
static inline void _read_region_name(char * name_buf , FILE * fs) {

    bool found_delim = false;

    //empty buffer
    memset(name_buf, 0, NAME_MAX);

    //only scan as many chars as a name can contain
    for (int i = 0; i < NAME_MAX; ++i) {

        name_buf[i] = _read_char(fs, false);
        if (name_buf[i] == DELIM_REGION) {
            found_delim = true;
            break;
        }
    }

    //fetch delimiter if not yet fetched
    if (!found_delim) _read_char(fs, false);
    
    return;
}


// --- PRIVATE METHODS

void serialiser::record_metadata(FILE * fs) {

    _record_value((cm_byte *) &this->bit_width, sizeof(this->bit_width), 1, fs);

    return;
}


void serialiser::read_metadata(FILE * fs) {

    _read_value((cm_byte *) &this->bit_width, sizeof(this->bit_width), 1, fs);

    return;
}


void serialiser::record_region_definitions(const mem * m, FILE * fs) {

    const cm_byte delim_region = DELIM_REGION;    
    const std::vector<cm_list_node *> * rw_areas = m->get_rw_areas();
    const std::vector<cm_list_node *> * static_areas = m->get_static_areas();
    const std::vector<cm_list_node *> * regions[2] = {rw_areas, static_areas};

    cm_list_node * vma_node;
    ln_vm_area * vma;
    ln_vm_obj * obj;
    char * vma_name;

    //for every region type
    for (int i = 0; i < 2; ++i) {

        //for every region
        for (int j = 0; j < regions[i]->size(); ++j) {

            vma_node = (*(regions[i]))[j];
            vma = LN_GET_NODE_AREA(vma_node);

            //record vma's own basename
            if (vma->basename != nullptr) {
                vma_name = vma->basename;
            //record last object's basename if vma has no basename
            } else {
                vma_node = vma->last_obj_node_ptr;
                obj = LN_GET_NODE_OBJ(vma_node);
                vma_name = obj->basename;
            }

            //record name
            _record_value((cm_byte *) vma_name, 
                          strnlen(vma_name, NAME_MAX), sizeof(char), fs);

            //record delimeter
            _record_value((cm_byte *) &delim_region, 1, sizeof(delim_region), fs);

        } //end for every region

        //record additional delimeter
        _record_value((cm_byte *) &delim_region, 1, sizeof(delim_region), fs);

    } //end for every region type

    return;
}


void serialiser::read_region_definitions(const mem * m, FILE * fs) {

    const char * exception_str[1] = {
        "serialise -> read_region_definitions: ungetc() failed."
    }; 

    int ret;

    int delim_region;
    char name_buf[NAME_MAX];

    cm_list_node * obj;


    //for every rw-/rwx area
    while ((delim_region = _read_char(fs, false)) != DELIM_REGION) {

        //push char back into stream
        ret = ungetc(delim_region, fs);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //read next region name
        _read_region_name(name_buf, fs);

        //find corresponding region
        obj = ln_get_vm_obj_by_basename(m->get_map(), name_buf);
        
        this->rw_objs.push_back(obj);

    } //end for every region

    //for every static area
    while ((delim_region = _read_char(fs, false)) != DELIM_REGION) {

        //push char back into stream
        ret = ungetc(delim_region, fs);
        if (ret) {
            throw std::runtime_error(exception_str[0]);
        }

        //read next region name
        _read_region_name(name_buf, fs);

        //find corresponding region
        obj = ln_get_vm_obj_by_basename(m->get_map(), name_buf);
        
        this->static_objs.push_back(obj);

    } //end for every region


    //read final delimiter (guaranteed to be DELIM_REGION)
    _read_char(fs, false);
    
    return;
}


void serialiser::record_offsets(const mem * m, FILE * fs) {

    const uint32_t delim_offset = DELIM_OFFSET;
    serial_entry * s_entry;

    //for every ptrchain
    for (int i = 0; i < this->ptrchains.size(); ++i) {

        s_entry = &this->ptrchains[i];
        
        //record rw-/rwx index
        _record_value((cm_byte *) &s_entry->rw_objs_index, 
                      sizeof(s_entry->rw_objs_index), 1, fs);

        //record static index
        _record_value((cm_byte *) &s_entry->static_objs_index, 
                      sizeof(s_entry->static_objs_index), 1, fs);

        //for every offset
        for (int j = 0; j < s_entry->offset_vector.size(); ++j) {

            //record offset 
            _record_value((cm_byte *) &s_entry->offset_vector[j], 
                          sizeof(s_entry->offset_vector[j]), 1, fs);
        
        } //end for every offset

        //record delimiter
        _record_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    } //end for every ptrchain

    //record double delimiter to indicate EOF
    _record_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    return;
}


void serialiser::read_offsets(const mem *, FILE * fs) {

    //FORMAT: [rw region id][static region id][offset 0]<...>[offset n][DELIM_OFFSET]

    uint32_t delim_offset;
    serial_entry s_entry;


    //bootstrap loop
    _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    //while more offset sets
    while (delim_offset != DELIM_OFFSET) {

        //set rw-/rwx index
        s_entry.rw_objs_index = delim_offset;

        //read static index
        _read_value((cm_byte *) &s_entry.static_objs_index,
                    sizeof(s_entry.static_objs_index), 1, fs);

        //bootstrap offset loop
        _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

        while (delim_offset != DELIM_OFFSET) {
            
            //push offset
            s_entry.offset_vector.push_back(delim_offset);

            //get next offset/delimiter
            _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

        } //end while offsets

        //get next rw-/rwx index/delimiter
        _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    } //end while more offset sets

}


//add offset from previous node to current node, producing an offset chain
void serialiser::recurse_offset(const mem_node * m_node, 
                                serial_entry * s_entry, const uintptr_t last_ptr) {

    //calculate this offset
    uint32_t offset = (uint32_t) (m_node->get_addr() - last_ptr);
    s_entry->offset_vector.push_back(offset);

    const mem_node * parent = m_node->get_parent();

    //recurse if this is not the root node
    if (parent != nullptr) {
        this->recurse_offset(parent, s_entry, m_node->get_ptr_addr());
    }

    return;
}


//recurse down the tree from root looking for leaves or static nodes
void serialiser::recurse_down(const args_struct * args, const mem * m, 
                              const mem_node * m_node, unsigned int current_depth) {

    int ret;
    bool stop_going_down;

    uint32_t offset;
   
    std::list<mem_node> * children;
    serial_entry s_entry;
    ln_vm_area * vma;

    /*
     *  stop going down if:
     *
     *  1) reached max depth
     *
     *  2) found a static node
     *
     *  3) current node has no children
     *
     */

    //if this node is not in a read & write region it is a false positive
    if (m_node->get_rw_areas_index() == -1) return;


    //check if should stop descending
    children = (std::list<mem_node> *) m_node->get_children();
    stop_going_down = (current_depth >= args->max_depth - 1)
                      || (m_node->get_static_areas_index() != -1)
                      || (children->size() == 0);


    //check if node is static or recursion reached max depth
    if (stop_going_down) {
        
        //setup new serial entry
        s_entry.rw_objs_index = m_node->get_rw_areas_index();
        s_entry.static_objs_index = m_node->get_static_areas_index();
        
        //calculate offset from backing object to node's address
        vma = LN_GET_NODE_AREA(m_node->get_vma_node());
        offset = (uint32_t) ln_get_obj_offset(vma->obj_node_ptr, m_node->get_addr());

        //add the offset
        s_entry.offset_vector.push_back(offset);

        //recursively traverse to root, adding offsets in the process
        this->recurse_offset(m_node->get_parent(), 
                             &s_entry, m_node->get_ptr_addr());

        //add this serial entry to the ptrchains vector
        this->ptrchains.push_back(s_entry);

    //else recurse for all child nodes
    } else {

        //for each child node
        for (std::list<mem_node>::iterator it = children->begin();
             it != children->end(); ++it) {

            //recurse down
            this->recurse_down(args, m, &(*it), current_depth + 1);
            
        } //end for
    } //end if-else

    return;
}


// --- PUBLIC METHODS

//convert mem_tree to a serialised format that can be stored and printed.
void serialiser::serialise_tree(const args_struct * args, 
                                const mem * m, const mem_tree * m_tree) {

    ln_vm_area * vma;

    const std::vector<cm_list_node *> * rw_areas = m->get_rw_areas();
    const std::vector<cm_list_node *> * static_areas = m->get_static_areas();


    //populate intermediate representation
    this->bit_width = args->bit_width;

    //for all rw-/rwx areas
    for (int i = 0; i < rw_areas->size(); ++i) {

        vma = LN_GET_NODE_AREA((*rw_areas)[i]);
        this->rw_objs.push_back(vma->obj_node_ptr);
    }
    
    //for all static areas
    for (int i = 0; i < static_areas->size(); ++i) {

        vma = LN_GET_NODE_AREA((*static_areas)[i]);
        this->static_objs.push_back(vma->obj_node_ptr);
    }

    //start recursive depth first traversal from root
    this->recurse_down(args, m, m_tree->get_root_node(), 0);

    return;
}


//write ptrchains to output .pscan file
void serialiser::record_pscan(const args_struct * args, const mem * m) {

    const char * exception_str[1] = {
        "serialise -> record_pscan: could not open output .pscan file for writing."
    }; 
    
    FILE * fs;
    

    //open output file
    fs = fopen(args->output_file.c_str(), "w");
    if (fs == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }

    //record metadata (bitwidth)
    record_metadata(fs);

    //record region definitions
    record_region_definitions(m, fs);

    //record offsets
    record_offsets(m, fs);


    //close output file
    fclose(fs);

    return;
}


//read ptrchains from input .pscan file
void serialiser::read_pscan(const args_struct * args, const mem * m) {

    const char * exception_str[1] = {
        "serialise -> record_pscan: could not open input .pscan file for reading."
    }; 
    
    FILE * fs;
    

    //open input file
    fs = fopen(args->input_file.c_str(), "r");
    if (fs == nullptr) {
        throw std::runtime_error(exception_str[0]);
    }

    //read metadata (bitwidth)
    read_metadata(fs);

    //read region defintions
    read_region_definitions(m, fs);

    //read offsets
    read_offsets(m, fs);


    //close input file
    fclose(fs);

    return;
}
