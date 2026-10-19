# Developer tools that need no title: built when REXBOX_BUILD_TOOLS is on.
# Tools that compare a title's HLE or decompiled code with its lifted
# originals belong to the title project.

add_executable(rexbox_xbe_info xbe_info/main.cpp)
target_link_libraries(rexbox_xbe_info PRIVATE rexbox_xbe rexbox_hle_kernel)
rexbox_check_license(rexbox_xbe_info)
rexbox_strict_warnings(rexbox_xbe_info)

# XDK symbol identification for scripts/recomp/regenerate.py.
add_executable(rexbox_xdk_symbols xdk_symbols/main.cpp)
target_link_libraries(rexbox_xdk_symbols PRIVATE libXbSymbolDatabase)
rexbox_check_license(rexbox_xdk_symbols)
rexbox_strict_warnings(rexbox_xdk_symbols)

# Kernel HLE checks that need no game data (run in CI).
add_executable(rexbox_kernel_probe kernel_probe/main.cpp)
target_link_libraries(rexbox_kernel_probe PRIVATE rexbox_hle_kernel)
rexbox_check_license(rexbox_kernel_probe)
rexbox_strict_warnings(rexbox_kernel_probe)

# DSOUND HLE checks that need no game data (run in CI).
add_executable(rexbox_dsound_probe dsound_probe/main.cpp)
target_link_libraries(rexbox_dsound_probe PRIVATE rexbox_hle_dsound)
rexbox_check_license(rexbox_dsound_probe)
rexbox_strict_warnings(rexbox_dsound_probe)

# D3D8 HLE checks of the pure parts (texture formats, FVF, fixed-function
# GLSL) that need no game data or GPU (run in CI). White-box: includes the
# module's own headers.
add_executable(rexbox_d3d8_probe d3d8_probe/main.cpp)
target_link_libraries(rexbox_d3d8_probe PRIVATE rexbox_hle_d3d8)
target_include_directories(rexbox_d3d8_probe PRIVATE ${rexbox_SOURCE_DIR}/src/hle/d3d8)
rexbox_check_license(rexbox_d3d8_probe)
rexbox_strict_warnings(rexbox_d3d8_probe)

# The probes are the unit tests: each exits 0 only with 0 failures.
add_test(NAME rexbox_kernel_probe COMMAND rexbox_kernel_probe)
add_test(NAME rexbox_dsound_probe COMMAND rexbox_dsound_probe)
add_test(NAME rexbox_d3d8_probe COMMAND rexbox_d3d8_probe)
