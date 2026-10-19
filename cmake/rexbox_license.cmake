# License guard. xboxrecomp is MIT, except its xemu-derived files, which are
# LGPL-2.1+ (its NOTICE). Those must never be compiled into rexbox or a title
# that uses it: we take xboxrecomp's tools and MIT sources only, file by file.
# Any source list that names one of these fails the configure (AGENTS.md).
#
# The paths are relative to the xboxrecomp checkout in use
# (REXBOX_XBOXRECOMP_DIR), so a consumer's own pin is guarded the same way.
set(REXBOX_COPYLEFT_PATHS
    "src/apu/"
    "src/nv2a/nv2a_core.c"
    "src/nv2a/nv2a_regs.h"
    "src/nv2a/nv2a_state.h"
    "include/xbox/xboxrecomp.h"
)
function(rexbox_check_license target)
    get_target_property(sources ${target} SOURCES)
    get_target_property(source_dir ${target} SOURCE_DIR)
    cmake_path(ABSOLUTE_PATH REXBOX_XBOXRECOMP_DIR NORMALIZE OUTPUT_VARIABLE xboxrecomp)
    foreach(source IN LISTS sources)
        cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY "${source_dir}" NORMALIZE OUTPUT_VARIABLE absolute)
        cmake_path(IS_PREFIX xboxrecomp "${absolute}" NORMALIZE inside)
        if(NOT inside)
            continue()
        endif()
        cmake_path(RELATIVE_PATH absolute BASE_DIRECTORY "${xboxrecomp}" OUTPUT_VARIABLE relative)
        foreach(blocked IN LISTS REXBOX_COPYLEFT_PATHS)
            string(FIND "${relative}" "${blocked}" hit)
            if(hit EQUAL 0)
                message(FATAL_ERROR "${target}: xboxrecomp/${relative} is copyleft-licensed and may not be built (AGENTS.md)")
            endif()
        endforeach()
    endforeach()
endfunction()
