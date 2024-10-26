#include <vector>
#include <string>
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
#include "debug.h"


#define TYPE_RW     0
#define TYPE_STATIC 1

// [!] makes rw-/rwx & static indexes of serial_entry iterable
#define ITER_INDEX(entry, type) (uint32_t *) \
                                ((((cm_byte *) entry) + (sizeof(uint32_t) * type)))


// --- INTERNAL

//wrapper for fwrite()
static inline void _record_value(cm_byte * value, 
                                 size_t size, size_t nmemb, FILE * fs) {

    const char * exception_str[1] = {
        "serialiser -> record_value: fwrite() request failed."
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
        "serialiser -> _read_value: fread() request failed."
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
        "serialiser -> _read_char: error reading .pscan file.",
        "serialiser -> _read_char: unexpected end of .pscan file."
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

    _record_value((cm_byte *) &this->byte_width, sizeof(this->byte_width), 1, fs);
    _record_value((cm_byte *) &this->alignment, sizeof(this->alignment), 1, fs);
    _record_value((cm_byte *) &this->max_struct_size, 
                  sizeof(this->max_struct_size), 1, fs);
    _record_value((cm_byte *) &this->max_depth, sizeof(this->max_depth), 1, fs);

    return;
}


void serialiser::read_metadata(FILE * fs) {

    _read_value((cm_byte *) &this->byte_width, sizeof(this->byte_width), 1, fs);
    _read_value((cm_byte *) &this->alignment, sizeof(this->alignment), 1, fs);
    _read_value((cm_byte *) &this->max_struct_size, 
                sizeof(this->max_struct_size), 1, fs);
    _read_value((cm_byte *) &this->max_depth, sizeof(this->max_depth), 1, fs);

    return;
}


void serialiser::record_region_definitions(const mem * m, FILE * fs) {

    const cm_byte delim_region = DELIM_REGION;    
    const std::vector<cm_list_node *> * rw_areas = &this->rw_objs;
    const std::vector<cm_list_node *> * static_areas = &this->static_objs;
    const std::vector<cm_list_node *> * regions[2] = {rw_areas, static_areas};

    cm_list_node * obj_node;
    ln_vm_obj * obj;

    //for every region type
    for (int i = 0; i < 2; ++i) {

        //for every region
        for (int j = 0; j < (int) regions[i]->size(); ++j) {

            obj_node = ((*regions)[i])[j];
            obj = LN_GET_NODE_OBJ(obj_node);

            //record name
            _record_value((cm_byte *) obj->basename, 
                          strnlen(obj->basename, NAME_MAX), sizeof(char), fs);

            //record delimeter
            _record_value((cm_byte *) &delim_region, 1, sizeof(delim_region), fs);

        } //end for every region

        //record additional delimeter
        _record_value((cm_byte *) &delim_region, 1, sizeof(delim_region), fs);

    } //end for every region type

    return;
}


void serialiser::read_region_definitions(const args_struct * args,
                                         const mem * m, FILE * fs) {

    const char * exception_str[1] = {
        "serialiser -> read_region_definitions: ungetc() failed."
    };

    int ret;

    int delim_region;
    char name_buf[NAME_MAX];

    std::vector<cm_list_node *> * objs[2] = {&this->rw_objs, &this->static_objs}; 
    std::vector<std::string> * read_objs[2] = {&this->read_rw_objs, 
                                               &this->read_static_objs};
    cm_list_node * obj_node;

    //for every type of object
    for (int i = 0; i < 2; ++i) {

        //for every area
        while ((delim_region = _read_char(fs, false)) != DELIM_REGION) {

            //push char back into stream
            ret = ungetc(delim_region, fs);
            if (ret == EOF) {
                throw std::runtime_error(exception_str[0]);
            }

            //read next region name
            _read_region_name(name_buf, fs);

            //if in read mode
            if (args->mode == MODE_READ) {
            
                read_objs[i]->push_back(name_buf);

            //else in verify mode, find corresponding region
            } else {
           
                obj_node = ln_get_vm_obj_by_basename(m->get_map(), name_buf);
                objs[i]->push_back(obj_node);
            
            } //end if

        } //end for every region

    } //end for every type of object
    
    return;
}


void serialiser::record_offsets(FILE * fs) {

    const uint32_t delim_offset = DELIM_OFFSET;
    serial_entry * s_entry;

    //for every ptrchain
    for (int i = 0; i < (int) this->ptrchains.size(); ++i) {

        s_entry = &this->ptrchains[i];
        
        //record rw-/rwx index
        _record_value((cm_byte *) &s_entry->rw_objs_index, 
                      sizeof(s_entry->rw_objs_index), 1, fs);

        //record static index
        _record_value((cm_byte *) &s_entry->static_objs_index, 
                      sizeof(s_entry->static_objs_index), 1, fs);

        //for every offset
        for (int j = 0; j < (int) s_entry->offsets.size(); ++j) {

            //record offset 
            _record_value((cm_byte *) &s_entry->offsets[j], 
                          sizeof(s_entry->offsets[j]), 1, fs);
        
        } //end for every offset

        //record delimiter
        _record_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    } //end for every ptrchain

    //record double delimiter to indicate EOF
    _record_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    return;
}


void serialiser::read_offsets(FILE * fs) {

    //FORMAT: [rw region id][static region id][offset 0]<...>[offset n][DELIM_OFFSET]

    uint32_t delim_offset;
    serial_entry s_entry;


    //bootstrap loop
    s_entry.basename = nullptr;
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
            s_entry.offsets.push_back(delim_offset);

            //get next offset/delimiter
            _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

        } //end while offsets

        this->ptrchains.push_back(s_entry);
        s_entry.offsets.clear();

        //get next rw-/rwx index/delimiter
        _read_value((cm_byte *) &delim_offset, sizeof(delim_offset), 1, fs);

    } //end while more offset sets

    return;
}


//identify rw-/rwx and static indexes for this serial entry to use
void serialiser::entry_to_disk_obj(serial_entry * s_entry) {

    std::vector<cm_list_node *> * objs[2] = {&this->rw_objs, &this->static_objs}; 
    ln_vm_obj * obj;

    //set initial value (no match)
    s_entry->rw_objs_index = s_entry->static_objs_index = -1;
    uint32_t * index;

    //for every type of object
    for (int i = 0; i < 2; ++i) {

        //for every object
        for (int j = 0; j < (int) objs[i]->size(); ++j) {
            
            //determine which object to give to this serial entry
            obj = LN_GET_NODE_OBJ((*objs[i])[j]);
            if (!strncmp(s_entry->basename, obj->basename, NAME_MAX)) {
                
                index = ITER_INDEX(s_entry, i);
                *index = j;    
                break;
            }
            
        } //end for every object

    } //end for every type of object

    return;
}


//add offset from previous node to current node, producing an offset chain
void serialiser::recurse_offset(const mem_node * m_node, 
                                serial_entry * s_entry, const uintptr_t last_ptr) {

    //calculate this offset
    uint32_t offset = (uint32_t) (m_node->get_addr() - last_ptr);
    s_entry->offsets.push_back(offset);

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

        //calculate offset from backing object to node's address & set basename
        vma = LN_GET_NODE_AREA(m_node->get_vma_node());
        if (vma->basename != nullptr) {
            offset = (uint32_t) 
                     ln_get_obj_offset(vma->obj_node_ptr, m_node->get_addr());
            s_entry.basename = vma->basename;

        } else {
            offset = (uint32_t) 
                     ln_get_obj_offset(vma->last_obj_node_ptr, m_node->get_addr());
            s_entry.basename = LN_GET_NODE_OBJ(vma->last_obj_node_ptr)->basename;
        }

        //determine disk backing objects for this serial entry
        this->entry_to_disk_obj(&s_entry);

        //add the offset
        s_entry.offsets.push_back(offset);

        //recursively traverse to root, adding offsets in the process
        if (current_depth != 0) this->recurse_offset(m_node->get_parent(), 
                                                     &s_entry, 
                                                     m_node->get_ptr_addr());

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


//verify an individual pointer chain
bool serialiser::verify_chain(const args_struct * args, 
                              const mem * m, const serial_entry * s_entry) {

    int ret;
    uintptr_t addr;

    cm_list_node * obj_node;
    ln_vm_obj * obj;

    //get start address
    obj_node = this->rw_objs[s_entry->rw_objs_index];
    obj = LN_GET_NODE_OBJ(obj_node);
    addr = obj->start_addr;
    
    //for each offset
    for (int i = 0; i < (int) s_entry->offsets.size(); ++i) {

        //add offset to address
        addr += s_entry->offsets[i];
        
        //read new address unless this is the last iteration
        if (i != (int) (s_entry->offsets.size() - 1)) {
            ret = ln_read(m->get_session(), addr, (cm_byte *) &addr, sizeof(addr));
            if (ret) {
                return false;
            }
        }

    } //end for each offset

    if (addr == args->target_addr) {
        return true;
    } else {
        return false;
    }
}


void serialiser::remove_unreferenced_objs() {

    bool referenced;

    std::vector<cm_list_node *> * objs[2] = {&this->rw_objs, &this->static_objs}; 
    serial_entry * s_entry;
    uint32_t * index;


    //for every type of object
    for (int i = 0; i < 2; ++i) {

        //for each object of this specific object type
        for (int j = 0; j < (int) objs[i]->size(); ++j) {

            referenced = false;

            //check if any pointer chain uses this object
            for (int k = 0; k < (int) this->ptrchains.size(); ++k) {

                s_entry = &this->ptrchains[k];
                index = ITER_INDEX(s_entry, i);

                //if this object has references
                if (*index == (uint32_t) j) referenced = true;
                
            } //end for each pointer chain

            //keep this object if it is referenced
            if (referenced) continue;

            //remove this object
            objs[i]->erase(objs[i]->begin() + j);

            //correct indexes of pointer chains
            for (int k = 0; k < (int) this->ptrchains.size(); ++k) {

                s_entry = &this->ptrchains[k];
                index = ITER_INDEX(s_entry, i);

                //
                if (*index >= (uint32_t) j) *index -= 1;
                
            } //end for each pointer chain
            
            //correct iteration
            j -= 1;

        } //end for each object

    } //end for each object type

    return;
}


//remove duplicate entries from object and static vectors 
void serialiser::remove_duplicate_objs() {

    cm_list_node * prev_obj_node, * curr_obj_node;
    ln_vm_obj * prev_obj, * curr_obj;

    std::vector<cm_list_node *> * objs[2] = {&this->rw_objs, &this->static_objs}; 


    //for every type of object
    for (int i = 0; i < 2; ++i) {

        //setup iteration
        prev_obj_node = (*objs[i])[0];
        prev_obj = LN_GET_NODE_OBJ(prev_obj_node);

        //for every object of a specific type
        for (int j = 1; j < (int) objs[i]->size(); ++j) {

            curr_obj_node = (*objs[i])[j];
            curr_obj = LN_GET_NODE_OBJ(curr_obj_node);

            //if this object's basename is the same as the last object's basename
            if (!strncmp(curr_obj->basename, prev_obj->basename, NAME_MAX)) {

                //delete duplicate object
                objs[i]->erase(objs[i]->begin() + j - 1);
                
                //correct iteration state
                prev_obj_node = (*objs[i])[j - 1];
                prev_obj = LN_GET_NODE_OBJ(prev_obj_node);
                j -= 1;
                continue;

            } //end if basename the same
            
            //increment prev iterator
            prev_obj = curr_obj;

        } //end for every object

    } //end for every type of object

    return;
}


// --- PUBLIC METHODS

//write ptrchains to output .pscan file
void serialiser::record_pscan(const args_struct * args, const mem * m) {

    const char * exception_str[1] = {
        "serialiser -> record_pscan: could not open output .pscan file for writing."
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
    record_offsets(fs);


    //close output file
    fclose(fs);

    return;
}


//read ptrchains from input .pscan file
void serialiser::read_pscan(const args_struct * args, const mem * m) {

    const char * exception_str[1] = {
        "serialiser -> record_pscan: could not open input .pscan file for reading."
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
    read_region_definitions(args, m, fs);

    //read offsets
    read_offsets(fs);


    //close input file
    fclose(fs);

    return;
}


//convert mem_tree to a serialised format that can be stored and printed.
void serialiser::serialise_tree(const args_struct * args, 
                                const mem * m, const mem_tree * m_tree) {

    ln_vm_area * vma;

    std::vector<cm_list_node *> * rw_areas;
    std::vector<cm_list_node *> * static_areas;

    //get iterable vector of mem areas
    rw_areas = (std::vector<cm_list_node *> *) m->get_rw_areas();
    static_areas = (std::vector<cm_list_node *> *) m->get_static_areas();
    std::vector<cm_list_node *> * areas[2] = {rw_areas, static_areas}; 
    
    //get iterable vector of serialiser objects
    std::vector<cm_list_node *> * objs[2] = {&this->rw_objs, &this->static_objs};


    //populate intermediate representation
    
    //set metadata
    this->byte_width      = args->byte_width;
    this->alignment       = args->alignment;
    this->max_struct_size = args->max_struct_size;
    this->max_depth       = args->max_depth;

    //for each type of area
    for (int i = 0; i < 2; ++i) {

        //for all areas of a specific type
        for (int j = 0; j < (int) areas[i]->size(); ++j) {

            vma = LN_GET_NODE_AREA((*areas[i])[j]);
            if (vma->basename != nullptr) {
                objs[i]->push_back(vma->obj_node_ptr);
            } else {
                objs[i]->push_back(vma->last_obj_node_ptr);
            }

        } //end for each area

    } //end for each type of area

    #ifdef DEBUG
    dump_serialiser_objs(&this->rw_objs);
    dump_serialiser_objs(&this->static_objs);
    #endif

    //remove duplicates
    this->remove_duplicate_objs();

    #ifdef DEBUG
    dump_serialiser_objs(&this->rw_objs);
    dump_serialiser_objs(&this->static_objs);
    #endif

    //start recursive depth first traversal from root
    this->recurse_down(args, m, m_tree->get_root_node(), 0);

    //removed unreferenced objects
    this->remove_unreferenced_objs();

    return;
}


//verify the intermediate representation by attempting to follow each chain
void serialiser::verify(const args_struct * args, const mem * m) {

    bool valid;

    serial_entry * s_entry;
    cm_list_node * obj_node;
    ln_vm_obj * obj;


    //for each pointer chain, verify the chain
    for (int i = 0; i < (int) this->ptrchains.size(); ++i) {

        s_entry  = &this->ptrchains[i];
        obj_node = this->rw_objs[s_entry->rw_objs_index];

        //if backing object for this chain could not be found
        if (obj_node == nullptr) {
            this->ptrchains.erase(this->ptrchains.begin() + i);
            --i;
            continue;
        }

        #ifdef DEBUG
        obj = LN_GET_NODE_OBJ(obj_node);
        #endif

        //if chain arrives at the wrong address
        valid = verify_chain(args, m, s_entry);
        if (!valid) {
            this->ptrchains.erase(this->ptrchains.begin() + i);
            --i;
            continue;
        }

    } //end for each pointer chain

    //clean up invalid backing objects
    remove_unreferenced_objs();

    return;
}


cm_byte serialiser::get_byte_width() const {
    return this->byte_width;
}

cm_byte serialiser::get_alignment() const {
    return this->alignment;
}

unsigned int serialiser::get_max_struct_size() const {
    return this->max_struct_size;
}

unsigned int serialiser::get_max_depth() const {
    return this->max_depth;
}

const std::vector<serial_entry> * serialiser::get_ptrchains() const {    
    return &this->ptrchains;
}

const std::vector<cm_list_node *> * serialiser::get_rw_objs() const {
    return &this->rw_objs;
}

const std::vector<cm_list_node *> * serialiser::get_static_objs() const {
    return &this->static_objs;
}

const std::vector<std::string> * serialiser::get_read_rw_objs() const {
    return &this->read_rw_objs;
}

const std::vector<std::string> * serialiser::get_read_static_objs() const {
    return &this->read_static_objs;
}
