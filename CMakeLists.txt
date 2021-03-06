cmake_minimum_required(VERSION 3.1)

########
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()
string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE )
########

########
# General project settings
project(poncos)
# Enable support for external projects
include(ExternalProject)

set(ENABLE_LOGGING ON CACHE BOOL "Enable logging.")
if(ENABLE_LOGGING)
	add_definitions(-DFASTLIB_ENABLE_LOGGING)
endif()
########

########
# Compiler/linker options based on the different compilers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # Using Clang
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -Ofast -march=native)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O1 -Weverything)

    ADD_DEFINITIONS(-fsanitize=address,undefined,integer -Wthread-safety -fno-omit-frame-pointer)
    SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined,integer")

    ADD_DEFINITIONS(-Wno-shadow)
  endif()

  # Options required to reduce the noise of spdlog
  ADD_DEFINITIONS(-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-sign-conversion -Wno-padded -Wno-switch-enum)
  ADD_DEFINITIONS(-Wno-old-style-cast -Wno-undef -Wno-documentation-unknown-command)
  ADD_DEFINITIONS(-Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors -Wno-newline-eof)
  ADD_DEFINITIONS(-Wno-missing-variable-declarations -Wno-double-promotion -Wno-extra-semi)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Using GCC
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -O2 -march=native)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O0 -Wall -Wextra -Wunused)
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # Using Intel C++
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -fast)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O0 -Wall -Wextra)
  endif()

  # CXX_STANDARD feature not working with older cmake + Intel
  ADD_DEFINITIONS(-std=c++11)

endif()
########

########
# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
########

########
# fast-lib
ExternalProject_Add(libfast
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast-lib
    CONFIGURE_COMMAND  CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast-lib
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Store libmosquitto variable in directory
ExternalProject_Get_Property(libfast install_dir)
set (libfast_path ${install_dir})
include_directories(${libfast_path}/include)
include_directories(${libfast_path}/include/fast-lib/external)
link_directories(${install_dir}/lib)
########

include_directories(include)

########
# Compiling and linking
add_executable(pons_macsnb src/poncos.cpp src/helper.cpp system_config/vm_pool.cpp src/job.cpp src/controller.cpp src/controller_cgroup.cpp src/controller_vm.cpp src/scheduler.cpp src/scheduler_two_app.cpp src/scheduler_multi_app.cpp src/scheduler_multi_app_consec.cpp src/system_config.cpp)
add_dependencies(pons_macsnb libfast)
target_link_libraries(pons_macsnb fastlib ${CMAKE_THREAD_LIBS_INIT} rt uuid)
set_property(TARGET pons_macsnb PROPERTY C_STANDARD 99)
set_property(TARGET pons_macsnb PROPERTY CXX_STANDARD 14)
########
