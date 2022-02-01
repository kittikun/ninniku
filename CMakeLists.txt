cmake_minimum_required(VERSION 3.20.0)

# Latest WinSDK
set(CMAKE_SYSTEM_VERSION 10.0)

project(ninniku CXX)

set(CMAKE_CONFIGURATION_TYPES "debug;release")

set(NINNIKU_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Each configuration outputs to its respective folder inside of bin/
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  set(OUTPUT_BIN "${NINNIKU_BIN_DIR}/${OUTPUTCONFIG}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_BIN})
endforeach()

# Constants
set(NINNIKU_ROOT_DIR  ${CMAKE_CURRENT_SOURCE_DIR})
set(NINNIKU_LIB_DIR ${NINNIKU_ROOT_DIR}/lib)
set(NINNIKU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Options
option(NINNIKU_AUTO_FORMAT "Run clang format" ON)

# Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(format_target)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

######################################################
## COMPILE AND LINK OPTIONS

set(WARNINGS_OPTIONS
  # Enable all warnings as errors
  -Wall -Wextra -Werror
  # List of warnings we want to disable
  -Wno-c++98-compat
  -Wno-c++20-compat
  -Wno-c++98-compat-pedantic)

set(BASE_COMPILE_OPTIONS
  -fms-compatibility
  $<$<CONFIG:debug>:	-g -O0>
  $<$<CONFIG:release>:	-O2 -flto>)

set(3RD_PARTY_COMPILE_OPTIONS
  -Wno-everything
  -fms-compatibility 
  -fno-rtti
  -O2 -flto)

set(NINNIKU_LINK_OPTIONS 
$<$<CONFIG:debug>:		-g>
$<$<CONFIG:release>:	-flto>)

set(NINNIKU_COMPILE_OPTIONS
  ${BASE_COMPILE_OPTIONS}
  ${WARNINGS_OPTIONS})

add_subdirectory(apps/simple)