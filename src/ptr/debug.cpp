#include <iostream>

#include <cstdint>

#include <linux/limits.h>

#include <libcmore.h>
#include <liblain.h>

#include "args.h"
#include "mem.h"
#include "mem_tree.h"
#include "thread_ctrl.h"
#include "thread.h"
#include "serialiser.h"


#define TAB   "\t"
#define NL    "\n"
#define SPACE " "


static inline void _dump_offsets(std::vector<uint32_t> offsets) {

    std::cerr << std::hex;

    //for every offset
    for (int i = 0; i < offsets.size(); ++i) {
        std::cerr << "+0x" << offsets[i] << SPACE;
    }

    std::cerr << std::dec << NL;

    return;
}


static inline void _dump_parent(const mem_node * m_node) {

    if (m_node->get_parent() == nullptr) {
        std::cerr << "<null>" << NL;
    } else {
        std::cerr << m_node->get_parent()->get_id() << NL;
    }

    return;
}


static inline void _dump_children(const std::list<mem_node> * children) {

    for (std::list<mem_node>::iterator it = 
         ((std::list<mem_node> *) children)->begin(); it != children->end(); ++it) {

        std::cerr << it->get_id() << SPACE;
    }

    std::cerr << NL;

}


static inline void _dump_access(cm_byte access) {

    const char letters[3] = {'r', 'w', 'x'};
    const char masks[3]   = {LN_ACCESS_READ, LN_ACCESS_WRITE, LN_ACCESS_EXEC};

    for (int i = 0; i < 3; ++i) {
        if (access & masks[i]) {
            std::cerr << letters[i];
        } else {
            std::cerr << '-';
        }
    }

    if (access & LN_ACCESS_SHARED) {
        std::cerr << 's';
    } else {
        std::cerr << 'p';
    }

    std::cerr << NL;

    return;
}


static inline void _dump_area_link(const cm_list_node * area_node) {

    ln_vm_area * area;
    
    area = LN_GET_NODE_AREA(area_node);
    std::cerr << area->id << NL;

    return;
}


//for ln_vm_obj's vma_area_node_ptrs
static inline void _dump_area_all_link(cm_list * objs) {

    cm_list_node * area_node;
    ln_vm_area * area;

    for (int i = 0; i < objs->len; ++i) {

        cm_list_get_val(objs, i, (cm_byte *) &area_node);
        area = LN_GET_NODE_AREA(area_node);

        std::cerr << area->id << SPACE;

    }

    std::cerr << NL;

    return;
}


static inline void _dump_obj_link(const cm_list_node * obj_node) {

    ln_vm_obj * obj;

    if (obj_node == nullptr) {
        std::cerr << "<null>" << NL;
    } else {
        obj = LN_GET_NODE_OBJ(obj_node);
        std::cerr << obj->id << NL;
    }

    return;
}


static inline void _dump_regions(std::string prefix, std::vector<region> regions) {

    region * r;

    for (int i = 0; i < regions.size(); ++i) {
        
        r = &regions[i];
        
        std::cerr << prefix << "  [region " << i << "]" << NL;
        std::cerr << prefix << TAB << "pathname:     " << r->pathname << NL;
        std::cerr << prefix << TAB << "skip/skipped: " << r->skip << "/" << r->skipped 
                                                       << NL;
    }

    return;
}


static inline void _dump_args_struct(std::string prefix, const args_struct * args) {

    const char * ui_types[2]  = {"TERM", "NCURSES"};
    const char * ln_ifaces[2] = {"LAINKO", "PROCFS"};

    std::cerr << prefix << "  [args_struct]" << NL << NL; 
    
    std::cerr << prefix << TAB << "target_str:         " << args->target_str << NL;
    std::cerr << prefix << TAB << "output_file:        " << args->output_file << NL;
    std::cerr << prefix << TAB << "input_file:         " << args->input_file << NL;
    std::cerr << prefix << NL;

    std::cerr << prefix << TAB << "ui_type:            " << ui_types[args->ui_type] 
                                                         << NL;
    std::cerr << prefix << TAB << "ln_iface:           " << ln_ifaces[args->ln_iface] 
                                                         << NL;
    std::cerr << NL;

    std::cerr << prefix << TAB << "colour:             " << args->colour << NL;
    std::cerr << prefix << TAB << "verbose:            " << args->verbose << NL;
    std::cerr << prefix << TAB << "use_preset_offsets: " << args->use_preset_offsets 
                                                         << NL;
    std::cerr << prefix << NL;

    std::cerr << prefix << TAB << "alignment:          " << args->alignment << NL;
    std::cerr << prefix << TAB << "bit_width:          " << args->bit_width << NL;
    std::cerr << prefix << TAB << "target_addr:        " << std::hex 
                                                         << args->target_addr 
                                                         << std::dec << NL;
    std::cerr << NL;

    std::cerr << prefix << TAB << "max_struct_size:    " << args->max_struct_size 
                                                         << NL;
    std::cerr << prefix << TAB << "max_depth:          " << args->max_depth << NL;
    std::cerr << prefix << TAB << "threads:            " << args->threads << NL;

    std::cerr << prefix << TAB << "extra_static_areas: " << NL << NL;
    _dump_regions(prefix + TAB, args->extra_static_areas);

    std::cerr << prefix << TAB << "exclusive_rw_areas: " << NL << NL;
    _dump_regions(prefix + TAB, args->exclusive_rw_areas);

    std::cerr << prefix << TAB << "preset offsets:     ";
    _dump_offsets(args->preset_offsets);
    std::cerr << NL;

    return;
}


static inline void _dump_areas(std::string prefix, std::vector<cm_list_node *> areas) {

    cm_list_node * area_node;
    ln_vm_area * area;

    for (int i = 0; i < areas.size(); ++i) {

        area_node = areas[i];
        area = LN_GET_NODE_AREA(area_node);

        std::cerr << prefix << "  [ln_vm_area]" << NL << NL;
        
        std::cerr << prefix << TAB << "pathname:          " << area->pathname << NL;
        std::cerr << prefix << TAB << "basename:          " << area->basename << NL;
        std::cerr << NL;

        std::cerr << prefix << TAB << "start_addr:        " << std::hex << "0x"
                                                            << area->start_addr
                                                            << std::dec << NL;
        std::cerr << prefix << TAB << "end_addr:          " << std::hex << "0x"
                                                            << area->end_addr
                                                            << std::dec << NL;
        std::cerr << NL;

        std::cerr << prefix << TAB << "access:            ";
        _dump_access(area->access);
        std::cerr << NL;

        std::cerr << prefix << TAB << "obj_node_ptr:      ";
        _dump_obj_link(area->obj_node_ptr);
        std::cerr << prefix << TAB << "last_obj_node_ptr: ";
        _dump_obj_link(area->obj_node_ptr);
        std::cerr << NL;

        std::cerr << prefix << TAB << "id:                " << area->id << NL;
        std::cerr << prefix << TAB << "mapped:            " << area->mapped << NL;
    }

    return;
}


static inline void _dump_objs(std::string prefix, 
                              const std::vector<cm_list_node *> * objs) {

    cm_list_node * obj_node;
    ln_vm_obj * obj;

    for (int i = 0; i < objs->size(); ++i) {

        obj_node = (*objs)[i];
        
        if (obj_node == nullptr) {
            std::cerr << prefix << "  [ln_vm_obj] <null>" << NL << NL;
            continue;
        }


        obj = LN_GET_NODE_OBJ(obj_node);

        std::cerr << prefix << "  [ln_vm_obj]" << NL << NL;

        std::cerr << prefix << TAB << "pathname:          " << obj->pathname << NL;
        std::cerr << prefix << TAB << "basename:          " << obj->basename << NL;
        std::cerr << NL;

        std::cerr << prefix << TAB << "start_addr:        " << std::hex << "0x"
                                                            << obj->start_addr
                                                            << std::dec << NL;
        std::cerr << prefix << TAB << "end_addr:          " << std::hex << "0x"
                                                            << obj->end_addr
                                                            <<std::dec << NL;
        std::cerr << NL;

        std::cerr << prefix << TAB << "id:                " << obj->id << NL;
        std::cerr << prefix << TAB << "mapped:            " << obj->mapped << NL;

        std::cerr << prefix << TAB << "vm_area_node_ptrs: ";
        _dump_area_all_link(&obj->vm_area_node_ptrs);
        std::cerr << NL;
    }

    return;
}


static inline void _dump_mem(std::string prefix, const mem * m) {

    std::cerr << prefix << "  [mem]" << NL << NL;

    std::cerr << prefix << TAB << "pid:          " << m->get_pid() << NL;
    std::cerr << prefix << TAB << "session:      " << std::hex << "0x" 
                                                   << (uintptr_t) m->get_session()
                                                   << std::dec << NL;
    std::cerr << prefix << TAB << "map:          " << std::hex << "0x" 
                                                   << (uintptr_t) m->get_map()
                                                   << std::dec << NL;
    
    std::cerr << prefix << TAB << "rw_areas:     " << NL << NL;
    _dump_areas(prefix + TAB, *m->get_rw_areas());
    std::cerr << prefix << TAB << "static_areas: " << NL << NL;
    _dump_areas(prefix + TAB, *m->get_static_areas());
    std::cerr << NL;

    return;
}


static inline void _dump_mem_node(std::string prefix, const mem_node * m_node) {

    std::cerr << prefix << "  [mem_node]" << NL << NL;

    std::cerr << prefix << TAB << "rw_areas_index:     " << m_node->
                                                            get_rw_areas_index()
                                                         << NL;
    std::cerr << prefix << TAB << "static_areas_index: " << m_node->
                                                            get_static_areas_index()
                                                         << NL;
    std::cerr << NL;

    std::cerr << prefix << TAB << "addr:               " << std::hex << "0x"
                                                         << m_node->get_addr()
                                                         << std::dec << NL;
    std::cerr << prefix << TAB << "ptr_addr:           " << std::hex << "0x"
                                                         << m_node->get_ptr_addr()
                                                         << std::dec << NL;
    std::cerr << NL;

    std::cerr << prefix << TAB << "vma_node:           ";
    _dump_area_link(m_node->get_vma_node());
    std::cerr << NL;

    std::cerr << prefix << TAB << "parent:             ";
    _dump_parent(m_node);
    std::cerr << prefix << TAB << "children:           ";
    _dump_children(m_node->get_children());


    return;
}


static inline void _dump_level(std::string prefix, 
                               std::list<mem_node *> * level, int index) {

    std::cerr << "  [level " << index << "]" << NL << NL;
    for (std::list<mem_node *>::iterator it = 
         level->begin(); it != level->end(); ++it) {

        _dump_mem_node(prefix + TAB, *it);
    }

    std::cerr << NL;
}


static inline void _dump_mem_tree(std::string prefix, const mem_tree * m_tree,
                                  const args_struct * args) {

    std::list<mem_node *> * level;

    std::cerr << prefix << "root_node: " << m_tree->get_root_node()->get_id() << NL;
    std::cerr << NL;

    std::cerr << prefix << "  [mem_tree]" << NL << NL;
    
    for (int i = 0; i < args->max_depth; ++i) {
        level = m_tree->get_level_list(i);
        _dump_level(prefix + TAB, level, i);
    }

    return;
}


static inline void _dump_vma_scan_ranges(std::string prefix, 
                                         const std::vector<vma_scan_range> * ranges) {

    vma_scan_range * range;

    for (int i = 0; i < ranges->size(); ++i) {
        
        range = &(* (std::vector<vma_scan_range> *) ranges)[i];

        std::cerr << prefix << "  [vma_scan_range " << i << "]" << NL << NL;

        std::cerr << prefix << TAB << "start_addr: " << std::hex << "0x"
                                                     << range->start_addr
                                                     << std::dec << NL;
        std::cerr << prefix << TAB << "end_addr:   " << std::hex << "0x"
                                                     << range->end_addr
                                                     << std::dec << NL;
        std::cerr << prefix << TAB << "vma_node:   ";
        _dump_area_link(range->vma_node);        
        std::cerr << NL;
    }

    return;
}


static inline void _dump_threads(std::string prefix, 
                                 const std::vector<thread> * threads) {

    thread * t;

    for (int i = 0; i < threads->size(); ++i) {

        t = &(* (std::vector<thread> *) threads)[i];
        
        std::cerr << prefix << "  [thread " << i << "]" << NL << NL;
        
        std::cerr << prefix << TAB << "ui_id:           " << t->get_ui_id() << NL;
        std::cerr << prefix << TAB << "vma_scan_ranges: " << NL << NL;
        _dump_vma_scan_ranges(prefix + TAB, t->get_vma_scan_ranges());
    }

    return;
}


static inline void _dump_thread_ctrl(std::string prefix, 
                                     const thread_ctrl * t_ctrl) {

    std::cerr << prefix << "  [thread_ctrl]" << NL << NL;

    _dump_threads(prefix + TAB, t_ctrl->get_threads());

    return;
}


static inline void _dump_serialiser(std::string prefix, const serialiser * s) {

    std::cerr << prefix << "  [serialiser]" << NL << NL;

    std::cerr << prefix << TAB << "bit_width: " << (int) s->get_bit_width() << NL;
    std::cerr << prefix << TAB << "ptrchains: " << "<see stdout>" << NL;
    std::cerr << NL;

    std::cerr << prefix << TAB << "rw_objs: " << NL;
    _dump_objs(prefix + TAB, s->get_rw_objs());

    return;
}


void __attribute__((noinline)) dump_args(const args_struct * args) {

    _dump_args_struct("", args);

    return;
}


void __attribute__((noinline)) dump_mem(const mem * m) {

    _dump_mem("", m);

    return;
}

void __attribute__((noinline)) dump_mem_tree(const mem_tree * m_tree,
                                             const args_struct * args) {

    _dump_mem_tree("", m_tree, args);

    return;
}

void __attribute__((noinline)) dump_threads(const thread_ctrl * t_ctrl) {

    _dump_thread_ctrl("", t_ctrl);

    return;
}

void __attribute__((noinline)) dump_serialiser(const serialiser * s) {

    _dump_serialiser("", s);

    return;
}
