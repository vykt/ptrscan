#ifndef UI_TERM_H
#define UI_TERM_H

#include <vector>
#include <string>

#include <cstdint>

#include <libcmore.h>
#include <liblain.h>

#include "ui_base.h"


#define RESET "\033[0m"
#define RED   "\033[31m" //error (?)
#define GREEN "\033[32m" //static



//virtual ui class, inherited from by terminal and tui ncurses interfaces
class ui_term : public ui_base {

    public:
        virtual void report_exception(const std::exception& e);
        virtual void report_depth_progress(const int depth_done);
        virtual void report_thread_progress(const unsigned int vma_done,
                                            const unsigned int vma_total,
                                            const int human_thread_id);
        
        virtual void output_ptrchains(const void * args_ptr,
                                      const void * s_ptr, const void * m_ptr);
};


#endif
