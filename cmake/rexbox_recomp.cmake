# The generated-code target of a title: the lifted C that
# scripts/recomp/regenerate.py wrote, compiled against the same lifter ABI
# header rexbox_recomp uses.
#
#   rexbox_add_recompiled_library(<target> GEN_DIR <dir> TITLE <title library>)
#
# Does nothing (and prints why) when <dir> holds no recomp_dispatch.c, so a
# tree without game data (CI) still configures; the caller tests
# `if(TARGET <target>)` for what depends on generated code. <title library>
# is the title's own target, which defines the decompiled sub_X functions the
# generated code calls and calls generated code back: a cycle between two
# static libraries, declared in both directions so linkers that scan each
# library once (GNU ld) repeat them; MSVC resolves the cycle on its own.
#
# Options: REXBOX_RECOMP_CALL_PROFILE (every direct call reports its target to
# recomp_call_profile_hit; DumpCallProfile prints it), REXBOX_RECOMP_ABI_CHECK
# (every direct call checks that the callee kept ebx/esi/edi and popped at
# least its return address). Both slow; off by default.
option(REXBOX_RECOMP_CALL_PROFILE "Build the generated code with per-call profiling (slow)" OFF)
option(REXBOX_RECOMP_ABI_CHECK "Build the generated code with per-call ABI checks (slow)" OFF)

function(rexbox_add_recompiled_library target)
    cmake_parse_arguments(ARG "" "GEN_DIR;TITLE" "" ${ARGN})
    if(NOT ARG_GEN_DIR OR NOT ARG_TITLE)
        message(FATAL_ERROR "rexbox_add_recompiled_library: GEN_DIR and TITLE are required")
    endif()
    if(NOT EXISTS "${ARG_GEN_DIR}/recomp_dispatch.c")
        message(STATUS "${target}: no recompiled code in ${ARG_GEN_DIR} (run scripts/recomp/regenerate.py)")
        return()
    endif()
    # The generated code must have been lifted against the same ABI header
    # rexbox_recomp compiles with; a different xboxrecomp pin means regenerate.
    file(SHA256 "${ARG_GEN_DIR}/recomp_types.h" gen_abi_hash)
    file(SHA256 "${REXBOX_XBOXRECOMP_DIR}/templates/runtime/recomp_types.h" pinned_abi_hash)
    if(NOT gen_abi_hash STREQUAL pinned_abi_hash)
        message(FATAL_ERROR "${ARG_GEN_DIR} was generated with a different xboxrecomp than "
                            "${REXBOX_XBOXRECOMP_DIR}; run scripts/recomp/regenerate.py")
    endif()

    file(GLOB sources CONFIGURE_DEPENDS "${ARG_GEN_DIR}/*.c")
    list(LENGTH sources count)
    message(STATUS "${target}: ${count} generated files from ${ARG_GEN_DIR}")

    add_library(${target} STATIC ${sources})
    target_include_directories(${target} PRIVATE
        "${ARG_GEN_DIR}"
        "${REXBOX_XBOXRECOMP_DIR}/src/kernel")  # recomp_icall_feedback.h (MIT)
    if(MSVC)
        # Machine-written code: warnings are noise, and the files are large.
        target_compile_options(${target} PRIVATE /w /bigobj /MP)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
    if(REXBOX_RECOMP_CALL_PROFILE)
        target_compile_definitions(${target} PRIVATE RECOMP_CALL_PROFILE)
    endif()
    if(REXBOX_RECOMP_ABI_CHECK)
        target_compile_definitions(${target} PRIVATE RECOMP_ABI_CHECK)
    endif()
    # An unresolved-target stub that runs reports itself once
    # (recomp_unresolved_stub_log in rexbox_recomp; the reJSRF fork of
    # xboxrecomp). Upstream xboxrecomp (v0.14+) reports from the stub itself
    # under RECOMP_STUB_LOG=1 and ignores this definition.
    target_compile_definitions(${target} PRIVATE RECOMP_STUB_REPORT)
    target_link_libraries(${target} PUBLIC rexbox_recomp ${ARG_TITLE})
    target_link_libraries(${ARG_TITLE} PUBLIC ${target})
    rexbox_check_license(${target})
endfunction()
