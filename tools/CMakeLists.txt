# Developer tools that need no title: built when REXBOX_BUILD_TOOLS is on.
# Tools that compare a title's HLE or decompiled code with its lifted
# originals belong to the title project.

add_executable(rexbox_xbe_info xbe_info/main.cpp)
target_link_libraries(rexbox_xbe_info PRIVATE rexbox_xbe rexbox_hle_kernel)
rexbox_strict_warnings(rexbox_xbe_info)

# XDK symbol identification (scripts/recomp/hle_overrides.py runs it).
add_executable(rexbox_xdk_symbols xdk_symbols/main.cpp)
target_link_libraries(rexbox_xdk_symbols PRIVATE libXbSymbolDatabase)
rexbox_strict_warnings(rexbox_xdk_symbols)

# Kernel HLE checks that need no game data (run in CI).
add_executable(rexbox_kernel_probe kernel_probe/main.cpp)
target_link_libraries(rexbox_kernel_probe PRIVATE rexbox_hle_kernel)
if(MSVC)
    # The physical-window check maps the RAM at base + 0x80000000 and
    # base + 0xF0000000 (modulo 2^32): a 32-bit process needs the 4 GiB
    # address space, as a title's application does.
    target_link_options(rexbox_kernel_probe PRIVATE /LARGEADDRESSAWARE)
endif()
rexbox_strict_warnings(rexbox_kernel_probe)

# DSOUND HLE checks that need no game data (run in CI).
add_executable(rexbox_dsound_probe dsound_probe/main.cpp)
target_link_libraries(rexbox_dsound_probe PRIVATE rexbox_hle_dsound)
rexbox_strict_warnings(rexbox_dsound_probe)

# D3D8 HLE checks of the pure parts (texture formats, FVF, fixed-function
# GLSL) that need no game data or GPU (run in CI). White-box: includes the
# module's own headers.
add_executable(rexbox_d3d8_probe d3d8_probe/main.cpp)
target_link_libraries(rexbox_d3d8_probe PRIVATE rexbox_hle_d3d8)
target_include_directories(rexbox_d3d8_probe PRIVATE ${rexbox_SOURCE_DIR}/src/hle/d3d8)
rexbox_strict_warnings(rexbox_d3d8_probe)

# The recompiled-code adapter in a title with no generated code (run in CI):
# the probe is the title and its application at once, linked through
# rexbox_add_recompiled_library with an empty GEN_DIR, so the register
# accessors rexbox_recomp calls come from recomp_abi_none.c.
add_executable(rexbox_recomp_probe recomp_probe/main.cpp)
target_link_libraries(rexbox_recomp_probe PRIVATE rexbox_recomp)
rexbox_add_recompiled_library(rexbox_recomp_probe_gen
    GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/recomp_probe/no_generated_code" TITLE rexbox_recomp_probe)
if(TARGET rexbox_recomp_probe_gen)
    message(FATAL_ERROR "rexbox_add_recompiled_library made a generated-code target without generated code")
endif()
rexbox_strict_warnings(rexbox_recomp_probe)

# The probes are the unit tests: each exits 0 only with 0 failures.
add_test(NAME rexbox_recomp_probe COMMAND rexbox_recomp_probe)
add_test(NAME rexbox_kernel_probe COMMAND rexbox_kernel_probe)
add_test(NAME rexbox_dsound_probe COMMAND rexbox_dsound_probe)
add_test(NAME rexbox_d3d8_probe COMMAND rexbox_d3d8_probe)
