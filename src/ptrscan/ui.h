#ifndef UI_H
#define UI_H

#include <string>

#include <libpwu.h>


//additional region to treat as static in addition to .bss & [stack]
typedef struct {

    std::string pathname; //backing file
    byte perms;           //region permissions
    int skip;             //if multiple regions with `pathname` backing file have
                          //`perms` permissions, skip the first `skip` entries
} extra_static_region;


#endif
