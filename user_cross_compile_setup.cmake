# Usage:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/user_cross_compile_setup.cmake
#   make -C build -j

# System we are targeting
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# -----------------------------------------------------------------------------
# OpenWrt-style SDK auto-detection
# - Expect an environment variable STAGING_DIR that points to the SDK staging_dir
# - If not set, try common defaults. You can export it before running CMake:
#     export STAGING_DIR=/opt/openwrt/staging_dir
# -----------------------------------------------------------------------------

if(NOT DEFINED ENV{STAGING_DIR})
    set(_owrt_guess "/opt/openwrt/staging_dir")
    if(EXISTS ${_owrt_guess})
        set(ENV{STAGING_DIR} ${_owrt_guess})
    endif()
endif()

if(NOT DEFINED ENV{STAGING_DIR})
    message(FATAL_ERROR "STAGING_DIR is not set. Please export STAGING_DIR to your OpenWrt SDK staging_dir.")
endif()

set(STAGING_DIR $ENV{STAGING_DIR})

# Locate toolchain-* and target-* inside STAGING_DIR
file(GLOB _toolchain_dirs "${STAGING_DIR}/toolchain-*")
file(GLOB _target_dirs    "${STAGING_DIR}/target-*")

list(LENGTH _toolchain_dirs _tc_len)
list(LENGTH _target_dirs _tg_len)
if(_tc_len EQUAL 0 OR _tg_len EQUAL 0)
    message(FATAL_ERROR "Could not find toolchain-* or target-* in ${STAGING_DIR}")
endif()

list(GET _toolchain_dirs 0 TOOLCHAIN_DIR)
list(GET _target_dirs 0   TARGET_DIR)

message(STATUS "Using TOOLCHAIN_DIR=${TOOLCHAIN_DIR}")
message(STATUS "Using TARGET_DIR=${TARGET_DIR}")

# Compilers - try common OpenWrt and GNU triplets for aarch64 first, then arm
set(_candidate_cc
    aarch64-openwrt-linux-gnu-gcc
    aarch64-openwrt-linux-musl-gcc
    aarch64-linux-gnu-gcc
    arm-openwrt-linux-gnueabi-gcc
    arm-linux-gnueabihf-gcc
)

set(_candidate_cxx
    aarch64-openwrt-linux-gnu-g++
    aarch64-openwrt-linux-musl-g++
    aarch64-linux-gnu-g++
    arm-openwrt-linux-gnueabi-g++
    arm-linux-gnueabihf-g++
)

unset(FOUND_CC CACHE)
unset(FOUND_CXX CACHE)
unset(_toolchain_triplet)

foreach(name IN LISTS _candidate_cc)
    if(NOT FOUND_CC)
        find_program(FOUND_CC NAMES ${name} PATHS "${TOOLCHAIN_DIR}/bin" NO_DEFAULT_PATH)
        if(FOUND_CC)
            if(name MATCHES "^aarch64")
                set(_toolchain_triplet aarch64)
            else()
                set(_toolchain_triplet arm)
            endif()
        endif()
    endif()
endforeach()

foreach(name IN LISTS _candidate_cxx)
    if(NOT FOUND_CXX)
        find_program(FOUND_CXX NAMES ${name} PATHS "${TOOLCHAIN_DIR}/bin" NO_DEFAULT_PATH)
    endif()
endforeach()

if(NOT FOUND_CC OR NOT FOUND_CXX)
    message(FATAL_ERROR "Could not find cross compilers in ${TOOLCHAIN_DIR}/bin. Tried: ${_candidate_cc}; ${_candidate_cxx}")
endif()

# Set compilers
set(CMAKE_C_COMPILER   ${FOUND_CC})
set(CMAKE_CXX_COMPILER ${FOUND_CXX})

# Adjust target processor based on detected triplet
if(DEFINED _toolchain_triplet AND _toolchain_triplet STREQUAL "aarch64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
else()
    set(CMAKE_SYSTEM_PROCESSOR arm)
endif()

# Sysroot
set(CMAKE_SYSROOT ${TARGET_DIR})
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config environment for cross build
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")

message(STATUS "Configured cross toolchain for OpenWrt (${CMAKE_SYSTEM_PROCESSOR})")

