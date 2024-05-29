cmake_minimum_required(VERSION 3.10)
project(mcas-benchmarks)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_FLAGS "-mrtm -Wall -fstack-protector -fstack-protector-all -DPARLAY_ALLOC_ALLOW_LEAK")
set(CMAKE_CXX_FLAGS "-mrtm -Wall -fstack-protector -fstack-protector-all -DPARLAY_ALLOC_ALLOW_LEAK")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "--------------- General configuration -------------")
message(STATUS "CMake Generator:                ${CMAKE_GENERATOR}")
message(STATUS "Compiler:                       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type:                     ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS:                ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:          ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE:        ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS          ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_INSTALL_PREFIX:           ${CMAKE_INSTALL_PREFIX}" )

option(BUILD_TESTS "If enabled, compile the tests." OFF)
if (BUILD_TESTS)
    find_package(GMock MODULE REQUIRED)
    find_package(GTest MODULE REQUIRED)
    include(GoogleTest OPTIONAL RESULT_VARIABLE HAVE_CMAKE_GTEST)
    enable_testing()
endif ()

if (MCAS_GEM5)
    add_definitions(-DMCAS_GEM5)
    add_definitions(-DENABLE_PARSEC_HOOKS)
endif ()

find_package(Threads REQUIRED)

if (HTM_BACKOFF)
    add_definitions(-DHTM_BACKOFF)
endif ()

file(GLOB LB_HEADER_FILES "lockbased/*.h")
file(GLOB LB_SOURCE_FILES "lockbased/*.cpp")

file(GLOB LF_HEADER_FILES "lockfree/*.h")
file(GLOB LF_SOURCE_FILES "lockfree/*.cpp")

file(GLOB LFMCAS_HEADER_FILES "lockfree-mcas/*.h")
file(GLOB LFMCAS_SOURCE_FILES "lockfree-mcas/*.cpp")

file(GLOB LFFLOCK_HEADER_FILES "lockfree-flock/*.h")
file(GLOB LFFLOCK_SOURCE_FILES "lockfree-flock/*.cpp")

file(GLOB LHTM_HEADER_FILES "htm-lock/*.h")
file(GLOB LHTM_SOURCE_FILES "htm-lock/*.cpp")

file(GLOB HTM_MCAS_HEADER_FILES "htm-mcas/*.h")
file(GLOB HTM_MCAS_SOURCE_FILES "htm-mcas/*.cpp")

file(GLOB FLOCK_MCAS_HEADER_FILES "flock-mcas/*.h")
file(GLOB FLOCK_MCAS_SOURCE_FILES "flock-mcas/*.cpp")

add_library(flock INTERFACE)
target_include_directories(flock INTERFACE ${PROJECT_SOURCE_DIR}/include/flock-lib)
target_compile_features(flock INTERFACE cxx_std_17)
target_compile_options(flock INTERFACE -mcx16 -march=native -g)

# Change `~/local/jemalloc/lib/` to path of your local installation if necessary
find_library(JEMALLOC_LIB jemalloc PATHS ~/local/jemalloc/lib/)
if(NOT JEMALLOC_LIB)
  message(FATAL_ERROR "Could not find jemalloc.")
else()
  message("-- Found jemalloc: ${JEMALLOC_LIB}")
  target_link_libraries(flock INTERFACE optimized ${JEMALLOC_LIB})
endif()

add_executable(data_structure_benchmarks main.cpp benchmarks.cpp benchmarks.h
               ${LB_HEADER_FILES} ${LB_SOURCE_FILES}
               ${LF_HEADER_FILES} ${LF_SOURCE_FILES}
               ${LFMCAS_HEADER_FILES} ${LFMCAS_SOURCE_FILES}
               ${LFFLOCK_HEADER_FILES} ${LFFLOCK_SOURCE_FILES}
               ${LHTM_HEADER_FILES} ${LHTM_SOURCE_FILES}
               ${HTM_MCAS_HEADER_FILES} ${HTM_MCAS_SOURCE_FILES}
               ${FLOCK_MCAS_HEADER_FILES} ${FLOCK_MCAS_SOURCE_FILES}
              )
target_include_directories(data_structure_benchmarks PRIVATE ${PROJECT_SOURCE_DIR}/include/htm PRIVATE ${PROJECT_SOURCE_DIR}/include/mcas)
target_link_libraries(data_structure_benchmarks ${CMAKE_THREAD_LIBS_INIT} flock)


#find_program(STATIC_ANALYZER pvs-studio)
#
#if (STATIC_ANALYZER)
#    include(cmake/PVS-Studio.cmake)
#    pvs_studio_add_target(TARGET analyze
#            ANALYZE mcas #subtarget:path/to/subtarget
#            FORMAT tasklist
#            PREPROCESSOR gcc
#            LOG "${CMAKE_HOME_DIRECTORY}/pvs.log")
#    #LICENSE "/path/to/PVS-Studio.lic"
#    #CXX_FLAGS ${PREPROCESSOR_ADDITIONAL_FLAGS}
#    #C_FLAGS ${PREPROCESSOR_ADDITIONAL_FLAGS}
#    #CONFIG "${CMAKE_HOME_DIRECTORY}/PVS-Studio.cfg"
#endif ()

