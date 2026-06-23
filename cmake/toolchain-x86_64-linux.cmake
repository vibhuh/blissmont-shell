# cmake/toolchain-x86_64-linux.cmake — native x86_64 Linux build.
#
# Native builds need no real cross-toolchain; the reproducibility lever is the vcpkg
# toolchain (wired via CMakePresets.json -> $env{VCPKG_ROOT}). This file exists to (a) match
# the spec's cmake/toolchain-*.cmake layout and (b) be the single place to pin a specific
# compiler if a build farm requires it. Pass it with:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-x86_64-linux.cmake ...
# (when combined with vcpkg, chainload it via VCPKG_CHAINLOAD_TOOLCHAIN_FILE instead).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Uncomment to pin an exact compiler on a build farm:
# set(CMAKE_C_COMPILER   gcc-13)
# set(CMAKE_CXX_COMPILER g++-13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
