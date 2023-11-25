#include <iostream>
#include <stdexcept>

#include <unistd.h>

#include <libpwu.h>

#include "ui_term.h"
#include "ui_base.h"



//report exception to user
inline void ui_term::report_exception(const std::exception& e) {

    std::cerr << e.what() << std::endl;
    return;
}


//get user to pick one PID from multiple matches
pid_t ui_term::clarify_pid(name_pid * n_pid) {

    const char * exception_str[1] = {
        "ui_term -> clarify_pid: vector range error."
    };

    int ret;
    std::string input;
    pid_t selection;
    pid_t * temp_pid;
    std::string buf;


    std::cout << "please select a PID matching the target name:" << std::endl;

    //for every match
    for (unsigned int i = 0; i < n_pid->pid_vector.length; ++i) {
        
        ret = vector_get_ref(&n_pid->pid_vector, (unsigned long) i,
                             (byte **) &temp_pid);
        if (ret == -1) {
            throw std::runtime_error(exception_str[0]);
        }

        std::cout << i << ": " << temp_pid << std::endl;
    }

    //ask until correct input is provided (they'll just type the PID i know it)
    while (1) {

        input.clear();
        std::getline(std::cin, input);
        try {
            selection = std::stoi(input);
        } catch (std::exception& e) {
            std::cout << "invalid selection provided";
        }
    } //end while

    //TODO then fix proc_mem::fetch_pid, it needs to call this
    return selection;
}
