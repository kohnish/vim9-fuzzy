configure_file(vim9_fuzzy_env.h.in ${PROJECT_BINARY_DIR}/include/vim9_fuzzy_env.h)

set(src
    ${proj_name}.c
    json_msg_handler.c
    search_helper.c
    fuzzy.c
    mru.c
    str_pool.c
    )

    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()

if(NOT WIN32)
    list(APPEND src
        yank.c
    )
endif()

set(lib
    uv
    jsmn
    )

if(WIN32)
    list(APPEND lib
                gdi32
                wsock32
                ws2_32
                gdi32
                wsock32
                ws2_32
                rpcrt4
                iphlpapi
                userenv
                psapi
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND lib
        dl
        pthread
        )
endif()

set(inc
    ${PROJECT_BINARY_DIR}/include
    )

add_executable(${proj_name}
    ${src}
        )

set(compile_opt
    -Wall
    -Wextra
    -pedantic
    -Werror
    -Wcast-align
    -Werror=vla
    )

target_compile_options(${proj_name} PRIVATE 
    ${compile_opt}
    )

target_include_directories(${proj_name} SYSTEM PRIVATE
    ${inc}
    )

target_link_libraries(${proj_name}
    ${lib}
    )

install(TARGETS ${proj_name} RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin/)

if(${BUILD_TEST})
    set(test_executable_name test-${proj_name})
    list(APPEND src
        ${test_executable_name}.cxx 
    )
    list(REMOVE_ITEM src
        ${proj_name}.c
    )
    list(APPEND lib
        gtest
        gtest_main
    )
    list(APPEND inc
        ${CMAKE_CURRENT_BINARY_DIR}/src
    )
    if(NOT ${USE_SYSTEM_GOOGLETEST})
        list(APPEND dep googletest)
    endif()
    add_executable(${test_executable_name}
        ${src}
    )
    target_compile_options(${test_executable_name} PRIVATE 
        ${compile_opt}
    )
    target_include_directories(${test_executable_name} SYSTEM PRIVATE
        ${inc}
    )
    target_link_libraries(${test_executable_name}
        ${lib}
    )
endif()
