# Toolchain for Orange Pi Zero 2W (aarch64) using local sysroot

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Avoid try-run on host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Resolve SYSROOT (fallback to project-local default if env not set)
if(NOT DEFINED ENV{SYSROOT})
    set(ENV{SYSROOT} "/home/kiwlee/cross/sysroots/sysroot_opi_zero_2w1G")
endif()

set(CMAKE_SYSROOT $ENV{SYSROOT})

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Enforce compiler to use the sysroot
set(CMAKE_C_FLAGS_INIT   "--sysroot=${CMAKE_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "--sysroot=${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Prefer fully static project libraries to avoid host GLIBC version pinning
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so" ".so.0" ".so.6")
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS "-Wl,-Bdynamic")
set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS "-Wl,-Bdynamic")

# Linker settings to ensure target linker/GLIBC are used (and avoid host libs)
set(_SYS_USR_LIB  "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu")
set(_SYS_LIB      "${CMAKE_SYSROOT}/lib/aarch64-linux-gnu")

set(_COMMON_LINK_FLAGS "-Wl,--dynamic-linker=/lib/ld-linux-aarch64.so.1 -Wl,-rpath-link,${_SYS_LIB} -Wl,-rpath-link,${_SYS_USR_LIB} -Wl,-Bdynamic -Wl,--as-needed -L${_SYS_LIB} -L${_SYS_USR_LIB}")
set(CMAKE_EXE_LINKER_FLAGS   "${CMAKE_EXE_LINKER_FLAGS} ${_COMMON_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS"${CMAKE_SHARED_LINKER_FLAGS} ${_COMMON_LINK_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS"${CMAKE_MODULE_LINKER_FLAGS} ${_COMMON_LINK_FLAGS}")

# Không chèn pthread sớm ở mức toolchain để tránh bị --as-needed loại bỏ; sẽ link ở target

# Avoid pulling host shared libs inadvertently; prefer static from sysroot
set(CMAKE_FIND_LIBRARY_PREFER_SHARED FALSE)

# Cho phép Threads module nhưng không cưỡng bức flags thủ công
set(THREADS_PREFER_PTHREAD_FLAG ON)

# pkg-config for cross
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_LIBDIR} "${_SYS_USR_LIB}/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")

message(STATUS "Cross toolchain: aarch64 with SYSROOT=${CMAKE_SYSROOT}")

