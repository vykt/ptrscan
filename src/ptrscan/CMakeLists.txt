message(STATUS "entering: ${CMAKE_CURRENT_SOURCE_DIR}")

#add sources
set(SOURCES_PTRSCAN 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/args.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/proc_mem.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_term.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/debug.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serialise.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verify.cpp
    PARENT_SCOPE)

set(HEADERS_PTRSCAN
    ${CMAKE_CURRENT_SOURCE_DIR}/args.h
    ${CMAKE_CURRENT_SOURCE_DIR}/proc_mem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_base.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_term.h
    ${CMAKE_CURRENT_SOURCE_DIR}/debug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_tree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_ctrl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialise.h
    ${CMAKE_CURRENT_SOURCE_DIR}/verify.h
    PARENT_SCOPE)
