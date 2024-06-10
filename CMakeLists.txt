cmake_minimum_required(VERSION 3.14.7)

project(remotechargeport
    VERSION 2024.05.0
    DESCRIPTION "EVerest modules for satellite charge ports"
    LANGUAGES CXX
)

find_package(everest-cmake 0.1
    COMPONENTS bundling
    PATHS ../everest-cmake
)

option(CREATE_SYMLINKS "Create symlinks to javascript modules and auxillary files - for development purposes" OFF)
option(CMAKE_RUN_CLANG_TIDY "Run clang-tidy" OFF)
option(BUILD_TESTING "Run unit tests" OFF)

# search for package rpclib
find_package(rpclib REQUIRED)

if(NOT DISABLE_EDM)
    # FIXME (aw): this implicit definition for child projects is hacky
    set(THIRD_PARTY_APP_DST "${CMAKE_INSTALL_LIBEXECDIR}/everest/3rd_party")

    evc_setup_edm()
else()
    find_package(everest-core REQUIRED)
endif()

ev_add_project()
