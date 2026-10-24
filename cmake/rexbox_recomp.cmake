# The generated-code target of a title: the lifted C the title's own
# recompilation pipeline wrote, together with rexbox's side of the lifter ABI.
#
#   rexbox_add_recompiled_library(<target> GEN_DIR <dir> TITLE <title library>)
#
# <dir> holds the lifted C and the lifter's ABI header, recomp_types.h. The
# lifter, its version and its pipeline are the title's; rexbox knows none of
# them (docs/design/20261024-i026-drop-xboxrecomp.md).
#
# When <dir> holds no recomp_dispatch.c, so a tree without game data (CI)
# still configures and links, it prints why, creates no <target> (the caller
# tests `if(TARGET <target>)` for what depends on generated code) and links
# <title library> to src/recomp/recomp_abi_none.c, the register accessors
# without the lifter's header. <title library>
# is the title's own target, which defines the decompiled sub_X functions the
# generated code calls and calls generated code back: a cycle between two
# static libraries, declared in both directions so linkers that scan each
# library once (GNU ld) repeat them; MSVC resolves the cycle on its own.
#
# src/recomp/recomp_abi.c, the definitions of the state and callbacks the
# generated code declares extern, is compiled into <target> against <dir>'s
# recomp_types.h, so it always matches the code it serves. Its object sits in
# the same archive as the generated code that references its state, so the
# register accessors rexbox_recomp calls are always linked.
#
# Options: REXBOX_RECOMP_CALL_PROFILE (every direct call reports its target to
# recomp_call_profile_hit; DumpCallProfile prints it), REXBOX_RECOMP_ABI_CHECK
# (every direct call checks that the callee kept ebx/esi/edi and popped at
# least its return address). Both slow; off by default.
option(REXBOX_RECOMP_CALL_PROFILE "Build the generated code with per-call profiling (slow)" OFF)
option(REXBOX_RECOMP_ABI_CHECK "Build the generated code with per-call ABI checks (slow)" OFF)
# The fork's SSE2 intrinsics for the XMM helpers are opt-in: by default every
# target that includes recomp_types.h gets the plain C helpers (the upstream
# design), so performance work stays on the HLE side until they are needed.
option(REXBOX_RECOMP_XMM_SSE2 "Build the recompiled code's XMM helpers with SSE2 intrinsics" OFF)

function(rexbox_add_recompiled_library target)
    cmake_parse_arguments(ARG "" "GEN_DIR;TITLE" "" ${ARGN})
    if(NOT ARG_GEN_DIR OR NOT ARG_TITLE)
        message(FATAL_ERROR "rexbox_add_recompiled_library: GEN_DIR and TITLE are required")
    endif()
    if(NOT EXISTS "${ARG_GEN_DIR}/recomp_dispatch.c")
        message(STATUS "${target}: no recompiled code in ${ARG_GEN_DIR} (run the title's recompilation pipeline)")
        # The application still links rexbox_recomp, which calls the register
        # accessors recomp_abi.c would define; recomp_abi_none.c defines them
        # without the lifter's header. <target> itself is not created, so the
        # title's `if(TARGET <target>)` still tells it there is no code.
        add_library(${target}_abi STATIC "${rexbox_SOURCE_DIR}/src/recomp/recomp_abi_none.c")
        target_link_libraries(${target}_abi PRIVATE rexbox_recomp)  # include/rexbox
        rexbox_strict_warnings(${target}_abi)
        target_link_libraries(${ARG_TITLE} PUBLIC ${target}_abi)
        # rexbox_recomp is what calls the accessors, so it must precede this
        # archive on the link line: linkers that scan each static library
        # once (GNU ld) resolve nothing from an archive placed before its
        # user. Declaring the dependency on rexbox_recomp orders it so; MSVC
        # never needed it (issue #31).
        target_link_libraries(rexbox_recomp PUBLIC ${target}_abi)
        return()
    endif()
    if(NOT EXISTS "${ARG_GEN_DIR}/recomp_types.h")
        message(FATAL_ERROR "${ARG_GEN_DIR} has no recomp_types.h, the lifter's ABI header the generated "
                            "code was written against; regenerate it")
    endif()

    file(GLOB sources CONFIGURE_DEPENDS "${ARG_GEN_DIR}/*.c")
    list(LENGTH sources count)
    message(STATUS "${target}: ${count} generated files from ${ARG_GEN_DIR}")

    # rexbox's side of the ABI: rexbox code, so rexbox's warnings.
    add_library(${target}_abi OBJECT "${rexbox_SOURCE_DIR}/src/recomp/recomp_abi.c")
    target_include_directories(${target}_abi PRIVATE "${ARG_GEN_DIR}" "${rexbox_SOURCE_DIR}/src/recomp")
    target_link_libraries(${target}_abi PRIVATE rexbox_recomp)  # include/rexbox
    rexbox_strict_warnings(${target}_abi)
    if(MSVC)
        # The lifter's header as its authors wrote it: its constant casts
        # (C4310, upstream v0.15.0's x87 helpers) are not ours to change.
        target_compile_options(${target}_abi PRIVATE /wd4310)
    endif()

    add_library(${target} STATIC ${sources} $<TARGET_OBJECTS:${target}_abi>)
    target_include_directories(${target} PRIVATE "${ARG_GEN_DIR}")
    if(MSVC)
        # Machine-written code: warnings are noise, and the files are large.
        target_compile_options(${target} PRIVATE /w /bigobj /MP)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
    # Every compilation that includes recomp_types.h must see the same
    # settings: the generated code, recomp_abi.c and (through the PUBLIC link
    # below) the title's decompiled sources.
    if(NOT REXBOX_RECOMP_XMM_SSE2)
        target_compile_definitions(${target} PUBLIC RECOMP_XMM_PORTABLE)
        target_compile_definitions(${target}_abi PRIVATE RECOMP_XMM_PORTABLE)
    endif()
    if(REXBOX_RECOMP_CALL_PROFILE)
        target_compile_definitions(${target} PRIVATE RECOMP_CALL_PROFILE)
        target_compile_definitions(${target}_abi PRIVATE RECOMP_CALL_PROFILE)
    endif()
    if(REXBOX_RECOMP_ABI_CHECK)
        target_compile_definitions(${target} PRIVATE RECOMP_ABI_CHECK)
        target_compile_definitions(${target}_abi PRIVATE RECOMP_ABI_CHECK)
    endif()
    # An unresolved-target stub that runs reports itself once
    # (recomp_unresolved_stub_log, defined in recomp_abi.c). A lifter that
    # reports from the stub itself ignores this definition.
    target_compile_definitions(${target} PRIVATE RECOMP_STUB_REPORT)
    target_link_libraries(${target} PUBLIC rexbox_recomp ${ARG_TITLE})
    target_link_libraries(${ARG_TITLE} PUBLIC ${target})
endfunction()
