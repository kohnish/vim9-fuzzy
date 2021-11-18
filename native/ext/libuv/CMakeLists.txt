if(${USE_SYSTEM_LIBUV})
    pkg_check_modules(UV libuv REQUIRED)
    add_library(uv SHARED IMPORTED GLOBAL)
    set_target_properties(uv PROPERTIES IMPORTED_LOCATION ${pkgcfg_lib_UV_uv})
else()
    ExternalProject_Add(libuv
        URL https://github.com/libuv/libuv/archive/refs/tags/v1.42.0.tar.gz
        DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/cache/libuv
        CMAKE_ARGS ${COMPILE_OPT} -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR} -DCMAKE_INSTALL_LIBDIR=${PROJECT_BINARY_DIR}/lib -DLIBUV_BUILD_TESTS=OFF
        URL_HASH SHA256=371e5419708f6aaeb8656671f89400b92a9bba6443369af1bb70bcd6e4b3c764
    )
    add_library(uv STATIC IMPORTED GLOBAL)
    set_target_properties(uv PROPERTIES IMPORTED_LOCATION ${PROJECT_BINARY_DIR}/lib/libuv_a.a)
    add_dependencies(uv libuv)
endif()
