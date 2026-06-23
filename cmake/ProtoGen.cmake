# ProtoGen.cmake — generate C++ + gRPC stubs for a consumed proto.
#
# Contract discipline (spec §3): terminal.proto is canonical in blissmont-contracts and
# pulled in as a submodule under third_party/. The shell NEVER edits it and NEVER commits
# the generated stubs — they are regenerated fresh on every build into a gitignored output
# dir, so the shell's stubs cannot drift from the engine's frozen contract. A contract
# change is fixed in blissmont-contracts first, the submodule bumped, and codegen re-run.
#
# Usage:
#   blissmont_add_proto_library(shell_proto
#     IMPORT_DIR <root that proto import paths are relative to>
#     OUT_DIR    <gitignored generated dir, e.g. ${CMAKE_SOURCE_DIR}/src/proto>
#     PROTOS     <abs path to each .proto>)

function(blissmont_add_proto_library TARGET)
  set(oneValueArgs IMPORT_DIR OUT_DIR)
  set(multiValueArgs PROTOS)
  cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_IMPORT_DIR OR NOT ARG_OUT_DIR OR NOT ARG_PROTOS)
    message(FATAL_ERROR "blissmont_add_proto_library: IMPORT_DIR, OUT_DIR and PROTOS are required")
  endif()

  # protoc from vcpkg's protobuf port; grpc_cpp_plugin from the gRPC CONFIG package.
  find_program(PROTOC_EXECUTABLE protoc REQUIRED)
  if(TARGET gRPC::grpc_cpp_plugin)
    set(_grpc_plugin "$<TARGET_FILE:gRPC::grpc_cpp_plugin>")
  else()
    find_program(_grpc_plugin grpc_cpp_plugin REQUIRED)
  endif()

  file(MAKE_DIRECTORY ${ARG_OUT_DIR})
  set(_generated_srcs "")

  foreach(_proto ${ARG_PROTOS})
    get_filename_component(_abs   "${_proto}" ABSOLUTE)
    get_filename_component(_name  "${_proto}" NAME_WE)
    file(RELATIVE_PATH _rel "${ARG_IMPORT_DIR}" "${_abs}")
    get_filename_component(_rel_dir "${_rel}" DIRECTORY)

    set(_base "${ARG_OUT_DIR}/${_rel_dir}/${_name}")
    set(_pb_cc   "${_base}.pb.cc")
    set(_pb_h    "${_base}.pb.h")
    set(_grpc_cc "${_base}.grpc.pb.cc")
    set(_grpc_h  "${_base}.grpc.pb.h")

    add_custom_command(
      OUTPUT  "${_pb_cc}" "${_pb_h}" "${_grpc_cc}" "${_grpc_h}"
      COMMAND ${PROTOC_EXECUTABLE}
              --proto_path=${ARG_IMPORT_DIR}
              --cpp_out=${ARG_OUT_DIR}
              --grpc_out=${ARG_OUT_DIR}
              --plugin=protoc-gen-grpc=${_grpc_plugin}
              ${_abs}
      DEPENDS "${_abs}" ${_grpc_plugin}
      COMMENT "protoc (cpp+grpc): ${_rel}"
      VERBATIM)

    list(APPEND _generated_srcs "${_pb_cc}" "${_grpc_cc}")
  endforeach()

  add_library(${TARGET} STATIC ${_generated_srcs})
  set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  # Consumers include e.g. <terminal/v1/terminal.pb.h> relative to OUT_DIR.
  target_include_directories(${TARGET} PUBLIC ${ARG_OUT_DIR})
  target_link_libraries(${TARGET} PUBLIC protobuf::libprotobuf gRPC::grpc++)
endfunction()
