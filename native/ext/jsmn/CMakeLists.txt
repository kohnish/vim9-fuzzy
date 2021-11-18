if(${USE_LOCAL_JSMN})
    ExternalProject_Add(libjsmn
        URL file:///${CMAKE_CURRENT_SOURCE_DIR}/jsmn.h
        CONFIGURE_COMMAND echo [INFO] No Configure
        BUILD_COMMAND echo [INFO] No Build
        INSTALL_COMMAND mkdir -p ${PROJECT_BINARY_DIR}/include && cp ${CMAKE_CURRENT_SOURCE_DIR}/jsmn.h ${PROJECT_BINARY_DIR}/include/jsmn.h
        DOWNLOAD_NO_EXTRACT 1
        URL_HASH SHA256=368dfcd4587163bf40ae2692529fb124a297320f7b113954ae79d28403fc2fc3
    )
    add_library(jsmn INTERFACE IMPORTED GLOBAL)
    add_dependencies(jsmn libjsmn)
else()
    ExternalProject_Add(libjsmn
        URL https://github.com/zserge/jsmn/archive/refs/tags/v1.1.0.tar.gz
        DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/cache/libjsmn
        CONFIGURE_COMMAND echo [INFO] No Configure
        BUILD_COMMAND echo [INFO] No Build
        INSTALL_COMMAND mkdir -p ${PROJECT_BINARY_DIR}/include && cp jsmn.h ${PROJECT_BINARY_DIR}/include/jsmn.h
        BUILD_IN_SOURCE 1
        URL_HASH SHA256=5f0913a10657fe7ec8d5794ccf00a01000e3e1f2f1e1f143c34a0f7b47edcb38
    )
    add_library(jsmn INTERFACE IMPORTED GLOBAL)
    add_dependencies(jsmn libjsmn)
endif()
