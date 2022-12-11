if(${USE_SYSTEM_LIBUV})
    pkg_check_modules(UV libuv REQUIRED)
    add_library(uv SHARED IMPORTED GLOBAL)
    set_target_properties(uv PROPERTIES IMPORTED_LOCATION ${pkgcfg_lib_UV_uv})
    set_target_properties(uv PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${UV_INCLUDE_DIRS})
else()
    set(uv_inst_prefix "${PROJECT_BINARY_DIR}/ext/libuv")
    ExternalProject_Add(libuv
        URL https://github.com/libuv/libuv/archive/refs/tags/v1.44.2.tar.gz
        DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/cache/libuv
        CMAKE_ARGS ${COMPILE_OPT} -DCMAKE_INSTALL_PREFIX=${uv_inst_prefix} -DLIBUV_BUILD_TESTS=OFF
        URL_HASH SHA256=e6e2ba8b4c349a4182a33370bb9be5e23c51b32efb9b9e209d0e8556b73a48da
        BUILD_BYPRODUCTS ${uv_inst_prefix}/lib/libuv_a.a
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    add_library(uv STATIC IMPORTED GLOBAL)
    add_dependencies(uv libuv)
    set_target_properties(uv PROPERTIES IMPORTED_LOCATION ${uv_inst_prefix}/lib/libuv_a.a)
    file(MAKE_DIRECTORY ${uv_inst_prefix}/include)
    set_target_properties(uv PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${uv_inst_prefix}/include)
endif()
