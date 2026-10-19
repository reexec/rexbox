# Compiler settings every target in a program that links rexbox must share.
# They are directory-scoped, so rexbox applies them to its own tree and a
# consumer calls rexbox_apply_host_flags() at its own root before adding its
# targets (CMake applies add_compile_options to the current directory and
# below only).
function(rexbox_apply_host_flags)
    if(WIN32)
        # _CRT_SECURE_NO_WARNINGS: portable code uses the standard C library
        # (getenv, fopen) rather than the MSVC-only _s variants.
        add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    endif()
    if(MSVC)
        # /utf-8: sources are UTF-8; without it MSVC reads them in the system
        # code page (C4819 on a Korean locale).
        add_compile_options(/utf-8 /permissive-)
        # /EHa: a guest thread ends (or stops) by unwinding native frames with
        # a structured exception (platform::RaiseStop); C++ destructors --
        # lock guards in HLE code among them -- must run during that unwind.
        # The whole program needs it, since the unwind crosses every layer.
        string(REPLACE "/EHsc" "/EHa" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86|x86_64|amd64|AMD64)$")
        # (A -m32 build on an x86-64 host still reports the host processor.)
        # 32-bit x86 with GCC or Clang: floating point on SSE, not the x87.
        # The x87 keeps intermediates at 80 bits, so a float expression
        # rounds differently from a stored float (two probe checks failed
        # that way), and lifted guest arithmetic must round as the Xbox's
        # Pentium III SSE and MSVC's x86 default (/arch:SSE2) do
        # (docs/design/20261020-i004-linux-preparation.md).
        add_compile_options(-msse2 -mfpmath=sse)
    endif()
endfunction()

# Warning level for code we write. Generated code is built with warnings off
# (rexbox_add_recompiled_library), so the level is per target, not global.
function(rexbox_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()
