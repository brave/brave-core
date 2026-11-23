# Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 OR ISC

# GenerateVersionScript.cmake
#
# Provides apply_version_script() to link a GNU ld version script into an ELF
# shared library target. Version scripts are maintained as checked-in source
# files (crypto/libcrypto.map, ssl/libssl.map) and generated from the symbol
# registries (crypto/libcrypto.txt, ssl/libssl.txt) via:
#
#   util/generate_initial_version_scripts.sh   (initial population)
#   util/update_symbol_version.sh <version>    (adding new symbols)

# Version-node prefix in the checked-in registries and .map files: their nodes
# are named AWS_LC_<major>.<minor>. -DSYMBOL_VERSION_NAMESPACE=<prefix> rewrites
# them into the build tree instead of using the script verbatim; see
# docs/SymbolVersioning.md for when that is appropriate. Unrelated to
# BORINGSSL_PREFIX: only node names change, never symbols, and the two are
# mutually exclusive (enforced in the top-level CMakeLists.txt).
set(AWSLC_DEFAULT_SYMBOL_VERSION_NAMESPACE "AWS_LC")

# A node name is "<namespace>_<major>.<minor>", so the namespace must be a valid
# linker identifier.
set(AWSLC_SYMBOL_VERSION_NAMESPACE_REGEX "^[A-Za-z_][A-Za-z0-9_]*$")

# _awslc_write_namespaced_version_script(<in_file> <namespace> <out_file>)
#
# Rewrites version-node names to a different namespace; symbol names are never
# touched. Done textually here rather than by invoking
# util/generate_version_script -namespace (which performs the same rename)
# because the checked-in .map files exist so that a build never has to run the
# generator, notably with -DDISABLE_GO=ON. run_symbol_version_test.sh diffs the
# two implementations against each other.
function(_awslc_write_namespaced_version_script in_file namespace out_file)
  set(default_ns "${AWSLC_DEFAULT_SYMBOL_VERSION_NAMESPACE}")

  file(READ "${in_file}" contents)

  # Node names appear only at the start of a line, as "AWS_LC_1.0 {" (the
  # declaration) and "} AWS_LC_1.0;" (the predecessor of an inheriting node).
  # CMake regexes have no multiline mode, hence the explicit "(^|\n)" anchor;
  # matching the line start is also what protects indented symbol names.
  string(REGEX REPLACE
    "(^|\n)${default_ns}_([0-9]+\\.[0-9]+)[ \t]*\\{"
    "\\1${namespace}_\\2 {"
    contents "${contents}")
  string(REGEX REPLACE
    "(^|\n)\\}[ \t]*${default_ns}_([0-9]+\\.[0-9]+)[ \t]*;"
    "\\1} ${namespace}_\\2;"
    contents "${contents}")

  # Never link a script that was not fully renamed. This check is unanchored so
  # it also catches a node name the rewrites could not reach (indented, or split
  # across lines): "<ns>_<digits>.<digits>" cannot occur in a symbol name ('.' is
  # invalid in an identifier) nor in an already-renamed node (AWS_LC_PRIVATE_1.0
  # has no digit directly after "AWS_LC_").
  if(contents MATCHES "${default_ns}_[0-9]+\\.[0-9]+")
    message(FATAL_ERROR
      "apply_version_script: failed to apply namespace '${namespace}' to "
      "${in_file}: one or more ${default_ns}_<major>.<minor> node names remain. "
      "Regenerate the version script with util/generate_initial_version_scripts.sh.")
  endif()
  # No node in the requested namespace means the input was not a version script.
  if(NOT contents MATCHES "(^|\n)${namespace}_[0-9]+\\.[0-9]+[ \t]*\\{")
    message(FATAL_ERROR
      "apply_version_script: no version node found in ${in_file}; expected at "
      "least one node named ${default_ns}_<major>.<minor> to rename to "
      "'${namespace}_<major>.<minor>'.")
  endif()

  # Write only on change: an unconditional write would bump the timestamp on
  # every configure and force a needless relink.
  set(write_needed TRUE)
  if(EXISTS "${out_file}")
    file(READ "${out_file}" existing)
    if(existing STREQUAL contents)
      set(write_needed FALSE)
    endif()
  endif()
  if(write_needed)
    file(WRITE "${out_file}" "${contents}")
  endif()

  # The rewrite happens at configure time, so re-run configure when the source
  # script changes (for example after regenerating it from the registry).
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${in_file}")
endfunction()

# apply_version_script()
#
# Applies a GNU ld version script to a library target for symbol versioning.
#
# Parameters:
#   TARGET         - Library target name (e.g., crypto, ssl)
#   VERSION_SCRIPT - Path to version script file (e.g., ${CMAKE_CURRENT_SOURCE_DIR}/libcrypto.map)
#   NAMESPACE      - Optional version-node prefix. Defaults to "AWS_LC", which uses
#                    VERSION_SCRIPT as-is; any other value rewrites the node names
#                    into a copy under the target's binary directory.

function(apply_version_script)
  set(options "")
  set(oneValueArgs TARGET VERSION_SCRIPT NAMESPACE)
  set(multiValueArgs "")
  cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

  if(NOT DEFINED ARG_TARGET)
    message(FATAL_ERROR "apply_version_script: TARGET is required")
  endif()

  if(NOT DEFINED ARG_VERSION_SCRIPT)
    message(FATAL_ERROR "apply_version_script: VERSION_SCRIPT is required")
  endif()

  if(NOT UNIX OR APPLE)
    message(STATUS "Symbol versioning not supported on this platform (requires GNU ld or compatible)")
    return()
  endif()

  if(NOT EXISTS "${ARG_VERSION_SCRIPT}")
    message(FATAL_ERROR "apply_version_script: Version script not found: ${ARG_VERSION_SCRIPT}")
  endif()

  # An empty NAMESPACE means "unspecified", not "no prefix".
  if("${ARG_NAMESPACE}" STREQUAL "")
    set(ARG_NAMESPACE "${AWSLC_DEFAULT_SYMBOL_VERSION_NAMESPACE}")
  endif()
  if(NOT "${ARG_NAMESPACE}" MATCHES "${AWSLC_SYMBOL_VERSION_NAMESPACE_REGEX}")
    message(FATAL_ERROR
      "apply_version_script: invalid NAMESPACE '${ARG_NAMESPACE}': a version "
      "node namespace must match ${AWSLC_SYMBOL_VERSION_NAMESPACE_REGEX}")
  endif()

  set(version_script "${ARG_VERSION_SCRIPT}")
  if(NOT "${ARG_NAMESPACE}" STREQUAL "${AWSLC_DEFAULT_SYMBOL_VERSION_NAMESPACE}")
    get_filename_component(script_name "${ARG_VERSION_SCRIPT}" NAME)
    set(version_script "${CMAKE_CURRENT_BINARY_DIR}/${script_name}")
    _awslc_write_namespaced_version_script(
      "${ARG_VERSION_SCRIPT}" "${ARG_NAMESPACE}" "${version_script}")
  endif()

  target_link_options(${ARG_TARGET} PRIVATE
    "LINKER:--version-script=${version_script}"
  )

  # The version script is passed via a linker flag, which CMake does not parse
  # to discover a build dependency. Record it explicitly so editing the .map
  # (e.g. regenerating it from the registry) triggers a re-link.
  set_target_properties(${ARG_TARGET} PROPERTIES
    LINK_DEPENDS "${version_script}"
  )

  if(LINKER_HAS_UNDEFINED_VERSION)
    target_link_options(${ARG_TARGET} PRIVATE
      "LINKER:--undefined-version"
    )
  endif()

  get_filename_component(VERSION_SCRIPT_NAME "${version_script}" NAME)
  if("${ARG_NAMESPACE}" STREQUAL "${AWSLC_DEFAULT_SYMBOL_VERSION_NAMESPACE}")
    message(STATUS "Applied symbol version script to ${ARG_TARGET}: ${VERSION_SCRIPT_NAME}")
  else()
    message(STATUS "Applied symbol version script to ${ARG_TARGET}: ${VERSION_SCRIPT_NAME} (namespace ${ARG_NAMESPACE})")
  endif()
endfunction()
