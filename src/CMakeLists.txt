# rexbox: the title-agnostic Xbox runtime. Nothing in this directory may name
# a title or include a title's headers; that is what lets any title reuse it
# unchanged.

# XBE image parsing. Pure data; no platform dependency.
add_library(rexbox_xbe STATIC
    xbe/xbe_image.cpp
)
target_include_directories(rexbox_xbe PUBLIC "${rexbox_SOURCE_DIR}/include")

# Runtime core: guest memory, XBE placement, the HLE registry.
add_library(rexbox_runtime STATIC
    runtime/environment.cpp
    runtime/guest_heap.cpp
    runtime/guest_lock.cpp
    runtime/guest_memory.cpp
    runtime/hle_call.cpp
    runtime/hle_registry.cpp
    runtime/thread_activity.cpp
    runtime/xbe_loader.cpp
)
target_link_libraries(rexbox_runtime PUBLIC rexbox_xbe rexbox_platform)

# Recompiled-code adapter: the bridge from generated code to the runtime and
# the executor that runs guest threads. It does not include the lifter's ABI
# header; recomp/recomp_abi.c, the one file that does, is compiled with each
# title's generated code (cmake/rexbox_recomp.cmake), and the register
# accessors it defines (include/rexbox/recomp/recomp_abi.h) resolve there.
add_library(rexbox_recomp STATIC
    recomp/recomp_bridge.cpp
    recomp/recomp_runtime.cpp
)
target_link_libraries(rexbox_recomp PUBLIC rexbox_runtime)

# HLE modules. Each links the runtime only, never another module, so any one
# can be replaced (or reused by another title) on its own.
add_library(rexbox_hle_kernel STATIC
    hle/kernel/kernel_apc.cpp
    hle/kernel/kernel_crypto.cpp
    hle/kernel/kernel_data.cpp
    hle/kernel/kernel_exports.cpp
    hle/kernel/kernel_file.cpp
    hle/kernel/kernel_hal.cpp
    hle/kernel/kernel_hle.cpp
    hle/kernel/kernel_memory.cpp
    hle/kernel/kernel_object.cpp
    hle/kernel/kernel_rtl.cpp
    hle/kernel/kernel_state.cpp
    hle/kernel/kernel_sync.cpp
    hle/kernel/kernel_timer.cpp
    hle/kernel/kernel_thread.cpp
    hle/kernel/kernel_vfs.cpp
)
target_link_libraries(rexbox_hle_kernel PUBLIC rexbox_runtime)

add_library(rexbox_hle_xapi STATIC hle/xapi/xapi_hle.cpp)
target_link_libraries(rexbox_hle_xapi PUBLIC rexbox_runtime)

# DirectSound on the platform audio contract (the APU it would drive is not
# emulated). One file per area; dsound_hle.cpp only registers.
add_library(rexbox_hle_dsound STATIC
    hle/dsound/dsound_3d.cpp
    hle/dsound/dsound_buffer.cpp
    hle/dsound/dsound_device.cpp
    hle/dsound/dsound_effects.cpp
    hle/dsound/dsound_hle.cpp
    hle/dsound/dsound_mixer.cpp
    hle/dsound/dsound_stream.cpp
)
target_link_libraries(rexbox_hle_dsound PUBLIC rexbox_runtime)

# XAPI's device and input functions on the platform input contract (the
# USB stack they would drive is not run). xinput_hle.cpp only registers.
add_library(rexbox_hle_xinput STATIC
    hle/xinput/xinput_devices.cpp
    hle/xinput/xinput_hle.cpp
    hle/xinput/xinput_state.cpp
)
target_link_libraries(rexbox_hle_xinput PUBLIC rexbox_runtime)

# Direct3D 8 on the platform display contract (OpenGL family). One file per
# area; d3d8_hle.cpp only registers.
add_library(rexbox_hle_d3d8 STATIC
    hle/d3d8/d3d8_adapter.cpp
    hle/d3d8/d3d8_caps.cpp
    hle/d3d8/d3d8_capture.cpp
    hle/d3d8/d3d8_device.cpp
    hle/d3d8/d3d8_display.cpp
    hle/d3d8/d3d8_ff_shader.cpp
    hle/d3d8/d3d8_fvf.cpp
    hle/d3d8/d3d8_gamma.cpp
    hle/d3d8/d3d8_gl.cpp
    hle/d3d8/d3d8_gl_buffers.cpp
    hle/d3d8/d3d8_gl_program_cache.cpp
    hle/d3d8/d3d8_gl_programs.cpp
    hle/d3d8/d3d8_gl_targets.cpp
    hle/d3d8/d3d8_gl_textures.cpp
    hle/d3d8/d3d8_globals.cpp
    hle/d3d8/d3d8_hle.cpp
    hle/d3d8/d3d8_immediate.cpp
    hle/d3d8/d3d8_profile.cpp
    hle/d3d8/d3d8_psh.cpp
    hle/d3d8/d3d8_resource.cpp
    hle/d3d8/d3d8_state.cpp
    hle/d3d8/d3d8_render_state.cpp
    hle/d3d8/d3d8_fog_blend.cpp
    hle/d3d8/d3d8_texture_format.cpp
    hle/d3d8/d3d8_trace.cpp
    hle/d3d8/d3d8_vblank.cpp
    hle/d3d8/d3d8_vsh.cpp
    hle/d3d8/d3d8_create.cpp
    hle/d3d8/d3d8_shader.cpp
    hle/d3d8/d3d8_smaa.cpp
    hle/d3d8/d3d8_draw.cpp
    hle/d3d8/d3d8_draw_watch.cpp
    hle/d3d8/d3d8_push.cpp
    hle/d3d8/d3d8_ltcg.cpp
    hle/d3d8/d3d8_post_process.cpp
    hle/d3d8/d3d8_query.cpp
    hle/d3d8/d3d8_stateblock.cpp
    hle/d3d8/post_shader_catalog.cpp
    hle/d3d8/post_shader_preset.cpp
    hle/d3d8/post_shader_source.cpp
)
target_link_libraries(rexbox_hle_d3d8 PUBLIC rexbox_runtime)

# SMAA (MIT, third_party/smaa): its shader source as a string header and its
# lookup textures (docs/design/20261017-i089-anti-aliasing.md).
include(embed_text)
embed_text(INPUT "${REXBOX_SMAA_DIR}/SMAA.hlsl" OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/smaa_source.h"
           NAME kSmaaSource)

# Built-in presentation post-processing shaders (issue #91).
set(REXBOX_POST_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hle/d3d8/post_shaders")
set(REXBOX_POST_SHADER_BUILTIN_NAMES
    crt.glsl
    scanline.glsl
    fsr/fsr.glslp
    fsr/easu.glsl
    fsr/rcas.glsl
    scalefx/scalefx.glslp
    scalefx/scalefx-pass0.glsl
    scalefx/scalefx-pass1.glsl
    scalefx/scalefx-pass2.glsl
    scalefx/scalefx-pass3.glsl
    scalefx/scalefx-pass4.glsl
    anime4k/anime4k.glslp
    anime4k/conv0.glsl
    anime4k/conv1.glsl
    anime4k/conv2.glsl
    anime4k/conv3.glsl
    anime4k/depth-to-space.glsl
)
set(REXBOX_POST_SHADER_BUILTIN_FILES "")
foreach(name IN LISTS REXBOX_POST_SHADER_BUILTIN_NAMES)
    set(path "${REXBOX_POST_SHADER_SOURCE_DIR}/${name}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
    file(READ "${path}" text)
    string(LENGTH "${text}" length)
    set(pieces "")
    set(offset 0)
    while(offset LESS length)
        string(SUBSTRING "${text}" ${offset} 12000 piece)
        string(APPEND pieces "R\"rexbox_glsl(${piece})rexbox_glsl\"\n")
        math(EXPR offset "${offset} + 12000")
    endwhile()
    string(APPEND REXBOX_POST_SHADER_BUILTIN_FILES "    {\"${name}\",\n${pieces}    },\n")
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${REXBOX_POST_SHADER_SOURCE_DIR}/post_shader_builtins.inc.in")
configure_file(
    "${REXBOX_POST_SHADER_SOURCE_DIR}/post_shader_builtins.inc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/post_shader_builtins.inc"
    @ONLY
)

target_include_directories(rexbox_hle_d3d8 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated" "${REXBOX_SMAA_DIR}/Textures")

add_library(rexbox_hle INTERFACE)
target_link_libraries(rexbox_hle INTERFACE
    rexbox_hle_kernel rexbox_hle_xapi rexbox_hle_d3d8 rexbox_hle_dsound rexbox_hle_xinput)
