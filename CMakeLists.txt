cmake_minimum_required(VERSION 3.20)

# rexbox: a title-agnostic Xbox runtime for statically recompiled titles.
# Built on its own (tools and probes) or as a subdirectory of a title project
# such as reJSRF or reCT3, which adds its own title layer and application.
#
# Consumer contract (docs/design/20261020-i001-project-bootstrap.md):
#   add_subdirectory(third_party/rexbox)
#   REXBOX_XBOXRECOMP_DIR   the xboxrecomp checkout whose lifter ABI header the
#                           generated code and rexbox_recomp compile against
#                           (default: rexbox's own pinned submodule)
#   REXBOX_BUILD_TOOLS      rexbox_xdk_symbols and the probes (default: when
#                           rexbox is the top-level project)
#   rexbox_apply_host_flags()            the compiler settings every target of
#                                        the program must share (cmake/rexbox_host_flags.cmake)
#   rexbox_add_recompiled_library(...)   the generated-code target (cmake/rexbox_recomp.cmake)
#   rexbox_check_license(), rexbox_strict_warnings()
#   REXBOX_PLATFORM, REXBOX_ARCH         the selected platform layer, exported to the parent

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" REXBOX_VERSION LIMIT_COUNT 1)
string(STRIP "${REXBOX_VERSION}" REXBOX_VERSION)
if(NOT REXBOX_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must use major.minor.patch format")
endif()

project(rexbox VERSION "${REXBOX_VERSION}" LANGUAGES C CXX)
message(STATUS "rexbox version ${REXBOX_VERSION}")

# Target platform and architecture. Platform code lives in
# src/platform/<platform>/ (and <platform>/<arch>/ when it differs by
# architecture); the runtime and HLE directories have none (AGENTS.md).
# Only win32/x86 is built and verified so far; the others are listed so the
# selection logic exists before their platform layers do.
if(EMSCRIPTEN)
    set(REXBOX_PLATFORM web)
    set(REXBOX_ARCH wasm32)
elseif(ANDROID)
    set(REXBOX_PLATFORM android)
    set(REXBOX_ARCH ${CMAKE_ANDROID_ARCH_ABI})
elseif(WIN32)
    set(REXBOX_PLATFORM win32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(REXBOX_ARCH x86)
    else()
        set(REXBOX_ARCH x64)
    endif()
elseif(UNIX)
    set(REXBOX_PLATFORM linux)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(REXBOX_ARCH x86)
    else()
        set(REXBOX_ARCH x64)
    endif()
else()
    message(FATAL_ERROR "rexbox targets Win32, Win64, Linux, the web and Android only")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${REXBOX_PLATFORM}")
    message(FATAL_ERROR
        "Platform '${REXBOX_PLATFORM}' has no platform layer yet (src/platform/${REXBOX_PLATFORM}/). "
        "Win32 x86 is the first target; see docs/PROJECT_CHARTER.md.")
endif()
message(STATUS "rexbox platform: ${REXBOX_PLATFORM}/${REXBOX_ARCH}")
if(NOT PROJECT_IS_TOP_LEVEL)
    set(REXBOX_PLATFORM "${REXBOX_PLATFORM}" PARENT_SCOPE)
    set(REXBOX_ARCH "${REXBOX_ARCH}" PARENT_SCOPE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(rexbox_host_flags)
include(rexbox_license)
include(rexbox_recomp)
rexbox_apply_host_flags()

option(REXBOX_BUILD_TOOLS "Build rexbox_xdk_symbols and the HLE probes" ${PROJECT_IS_TOP_LEVEL})

# The xboxrecomp checkout supplies the lifter's ABI header
# (templates/runtime/recomp_types.h) to rexbox_recomp and the pipeline tools
# to scripts/recomp/. A consumer that pins its own xboxrecomp points this at
# it; the generated code and rexbox_recomp must come from the same checkout.
set(REXBOX_XBOXRECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xboxrecomp" CACHE PATH
    "xboxrecomp checkout whose lifter ABI header rexbox_recomp compiles against")
if(NOT EXISTS "${REXBOX_XBOXRECOMP_DIR}/templates/runtime/recomp_types.h")
    message(FATAL_ERROR "${REXBOX_XBOXRECOMP_DIR} has no templates/runtime/recomp_types.h; "
                        "run: git submodule update --init (or set REXBOX_XBOXRECOMP_DIR)")
endif()

# SDL3 (zlib): the host library behind the display, input and audio contracts
# on every target OS. Static library only; subsystems no HLE module uses stay
# off. Audio serves the DSOUND HLE, joystick and gamepad the XInput HLE.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/CMakeLists.txt")
    message(FATAL_ERROR "third_party/SDL is missing; run: git submodule update --init")
endif()
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "" FORCE)
foreach(subsystem IN ITEMS GPU RENDER CAMERA HAPTIC SENSOR DIALOG TRAY)
    set(SDL_${subsystem} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)

# Dear ImGui (MIT): the overlay. The core is a library a consumer's app builds
# its screens with; the SDL3 and OpenGL3 backends compile inside
# rexbox_platform, which owns SDL.
set(REXBOX_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
if(NOT EXISTS "${REXBOX_IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "third_party/imgui is missing; run: git submodule update --init")
endif()
add_library(imgui STATIC
    "${REXBOX_IMGUI_DIR}/imgui.cpp"
    "${REXBOX_IMGUI_DIR}/imgui_draw.cpp"
    "${REXBOX_IMGUI_DIR}/imgui_tables.cpp"
    "${REXBOX_IMGUI_DIR}/imgui_widgets.cpp")
target_include_directories(imgui PUBLIC "${REXBOX_IMGUI_DIR}")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# SMAA (MIT): shader source embedded by the D3D8 HLE.
set(REXBOX_SMAA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/smaa")
if(NOT EXISTS "${REXBOX_SMAA_DIR}/SMAA.hlsl")
    message(FATAL_ERROR "third_party/smaa is missing; run: git submodule update --init")
endif()

if(REXBOX_BUILD_TOOLS)
    # XbSymbolDatabase (MIT) identifies statically linked XDK functions for
    # the rexbox_xdk_symbols tool; only its library is built.
    set(XBSDB_BUILD_CLI OFF CACHE BOOL "" FORCE)
    set(XBSDB_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
    set(XBSDB_INSTALL_LIB OFF CACHE BOOL "" FORCE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/XbSymbolDatabase/CMakeLists.txt")
        message(FATAL_ERROR "third_party/XbSymbolDatabase is missing; run: git submodule update --init")
    endif()
    add_subdirectory(third_party/XbSymbolDatabase EXCLUDE_FROM_ALL)
endif()

# Layers, lowest first. Each layer links only the layers below it.
add_subdirectory(src/platform)       # rexbox_platform: OS contracts
add_subdirectory(src)                # rexbox_*: XBE, runtime, recomp adapter, HLE modules
if(REXBOX_BUILD_TOOLS)
    add_subdirectory(tools)          # rexbox_xdk_symbols, rexbox_xbe_info, probes
endif()

set(REXBOX_OWN_TARGETS
    rexbox_platform rexbox_xbe rexbox_runtime rexbox_recomp
    rexbox_hle_kernel rexbox_hle_xapi rexbox_hle_d3d8 rexbox_hle_dsound rexbox_hle_xinput)
foreach(target IN LISTS REXBOX_OWN_TARGETS)
    rexbox_check_license(${target})
    rexbox_strict_warnings(${target})
endforeach()
