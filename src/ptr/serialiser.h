#ifndef SERIALISER_H
#define SERIALISER_H

#include <vector>

#include <cstdint>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "mem_tree.h"


#define EOT          0x3 //end of text
#define DELIM_REGION 0
#define DELIM_OFFSET 0xFFFFFFF0


//single pointer chain
typedef struct {

    // [!] indexes must be first 2 elements of serial_entry
    uint32_t rw_objs_index;
    uint32_t static_objs_index;

    std::vector<uint32_t> offsets;

    //for use during generation
    char * basename;

} serial_entry;



//-serialise the tree in memory
//-write tree to disk
//-read a tree from disk
class serialiser {

    private:
        //attributes
        cm_byte byte_width;
        std::vector<serial_entry> ptrchains;

        //intermediate representation (vectors can contain null ptrs)
        std::vector<cm_list_node *> rw_objs;
        std::vector<cm_list_node *> static_objs;


    private:
        //methods
        void record_metadata(FILE * fs);
        void read_metadata(FILE * fs);
        void record_region_definitions(const mem * m, FILE * fs);
        void read_region_definitions(const mem * m, FILE * fs);
        void record_offsets(const mem * m, FILE * fs);
        void read_offsets(const mem * m, FILE * fs);

        void entry_to_disk_obj(serial_entry * s_entry);
        void recurse_offset(const mem_node * m_node, 
                            serial_entry * s_entry, const uintptr_t last_ptr);
        void recurse_down(const args_struct * args, const mem * m, 
                          const mem_node * m_node, unsigned int current_depth);
        bool verify_chain(const args_struct * args, 
                          const mem * m, const serial_entry * s_entry);
        void remove_unreferenced_objs();
        void remove_duplicate_objs();

    public:
        //methods
        void record_pscan(const args_struct * args, const mem * m);
        void read_pscan(const args_struct * args, const mem * m);
        void serialise_tree(const args_struct * args, 
                            const mem * m, const mem_tree * m_tree);
        void verify(const args_struct * args, const mem * m);

        //getters & setters
        cm_byte get_byte_width() const;
        const std::vector<serial_entry> * get_ptrchains() const;
        const std::vector<cm_list_node *> * get_rw_objs() const;
        const std::vector<cm_list_node *> * get_static_objs() const;
};


#endif
