#ifndef UI_BASE_H
#define UI_BASE_H

#include <vector>
#include <string>

#include <libpwu.h>


#define UI_TERM 0
#define UI_NCURSES 1


//additional region to treat as static in addition to .bss & [stack]
typedef struct {

    std::string pathname; //backing file
    int skip;             //if multiple regions with `pathname` backing file have
    int skipped;          //`perms` permissions, skip the first `skip` entries

} static_region;


//abstract ui class, inherited from by terminal and tui ncurses interfaces
class ui_base {

    public:
        //methods
        virtual void report_exception(const std::exception& e) = 0;
        virtual pid_t clarify_pid(name_pid * n_pid) = 0;

};


#endif
