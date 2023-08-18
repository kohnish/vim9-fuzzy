if(${USE_SYSTEM_GOOGLETEST})
    pkg_check_modules(GTEST gtest REQUIRED)
    add_library(gtest SHARED IMPORTED GLOBAL)
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION ${pkgcfg_lib_GTEST_gtest})
    if (GTEST_INCLUDE_DIRS)
        set_target_properties(gtest PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS})
    endif()

    pkg_check_modules(GTEST_MAIN gtest_main REQUIRED)
    add_library(gtest_main SHARED IMPORTED GLOBAL)
    set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION ${pkgcfg_lib_GTEST_MAIN_gtest_main})
    if (GTEST_MAIN_INCLUDE_DIRS)
        set_target_properties(gtest_main PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GTEST_MAIN_INCLUDE_DIRS})
    endif()

    pkg_check_modules(GMOCK gmock REQUIRED)
    add_library(gmock SHARED IMPORTED GLOBAL)
    set_target_properties(gmock PROPERTIES IMPORTED_LOCATION ${pkgcfg_lib_GMOCK_gmock})
    if (GMOCK_INCLUDE_DIRS)
        set_target_properties(gmock PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GMOCK_INCLUDE_DIRS})
    endif()
else()
    set(googletest_prefix ${PROJECT_BINARY_DIR}/googletest)
    ExternalProject_Add(googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
        DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/cache/googletest
        URL_HASH SHA256=b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5
        CMAKE_ARGS ${COMPILER_OPT} -DCMAKE_INSTALL_PREFIX=${googletest_prefix} -DCMAKE_INSTALL_LIBDIR=${googletest_prefix}/lib
        BUILD_BYPRODUCTS ${googletest_prefix}/lib/libgtest.a ${googletest_prefix}/lib/libgtest_main.a
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    add_library(gtest STATIC IMPORTED GLOBAL)
    add_library(gtest_main STATIC IMPORTED GLOBAL)
    add_library(gmock STATIC IMPORTED GLOBAL)
    set_target_properties(gtest PROPERTIES IMPORTED_LOCATION ${googletest_prefix}/lib/libgtest.a)
    set_target_properties(gmock PROPERTIES IMPORTED_LOCATION ${googletest_prefix}/lib/libgmock.a)
    set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION ${googletest_prefix}/lib/libgtest_main.a)
    file(MAKE_DIRECTORY ${googletest_prefix}/include)
    set_target_properties(gtest PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${googletest_prefix}/include)
    set_target_properties(gtest_main PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${googletest_prefix}/include)
    set_target_properties(gmock PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${googletest_prefix}/include)
    add_dependencies(gtest googletest)
    add_dependencies(gtest_main googletest)
    add_dependencies(gmock googletest)
endif()
