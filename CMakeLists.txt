# blissmont-shell — top-level build.
# Qt 6.8 LTS + C++20. Third-party C++ deps (protobuf/grpc/gtest) via vcpkg manifest;
# Qt is found on CMAKE_PREFIX_PATH (installed separately — Qt online installer / aqt / distro).
#
# Configure with the vcpkg toolchain + a Qt prefix, e.g.:
#   cmake --preset linux-debug -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.0/gcc_64
#   cmake --build --preset linux-debug
cmake_minimum_required(VERSION 3.21)

project(blissmont-shell
  VERSION 0.1.0
  DESCRIPTION "BlissMont POS shell — Qt/C++ thin client over the terminal engine"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(BLISSMONT_BUILD_TESTS "Build the libshell unit + integration tests" ON)

# ── Qt (installed separately; LTS pinned) ─────────────────────────────────────
# Test = QtTest (QSignalSpy), used by the unit + integration suites.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Test)
qt_standard_project_setup(REQUIRES 6.8)
set(CMAKE_AUTOMOC ON)

# ── protobuf + gRPC (vcpkg) ───────────────────────────────────────────────────
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# ── Codegen for the consumed (never edited) terminal.proto ────────────────────
include(cmake/ProtoGen.cmake)

# ── Library-first: core/bridge/models/viewmodels/services -> libshell ─────────
add_subdirectory(src)

if(BLISSMONT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
