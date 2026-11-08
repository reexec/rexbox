// Linked GL programs kept on disk across runs
// (docs/design/20261014-i051-program-cache.md).
//
// Compiling and linking a program takes tens of milliseconds on some
// drivers, and the title asks for new programs in the middle of scenes. A
// program the driver can hand back as a binary (ARB_get_program_binary,
// core in GL 4.1 and GLES 3.0) is stored with the driver's identity and the
// full sources; the next run creates it from the binary when both match, and
// compiles as usual when there is no entry or the driver rejects it.
//
// REXBOX_SHADER_CACHE=<dir> chooses the directory (default cache/shaders in
// the current folder); REXBOX_SHADER_CACHE=0 turns the cache off.

#ifndef REXBOX_HLE_D3D8_D3D8_GL_PROGRAM_CACHE_H
#define REXBOX_HLE_D3D8_D3D8_GL_PROGRAM_CACHE_H

#include "d3d8_profile.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace rexbox::hle::d3d8
{

// Whether programs are cached: on, and the driver offers binary formats.
bool ProgramCacheEnabled();

// A linked program made from the stored binary of these sources, or 0.
// When loaded, `origin` (binary cache or warmed) and `made_time` are reported if requested.
uint32_t LoadCachedProgram(const std::string& vertex_source, const std::string& fragment_source,
                           profile::ProgramOrigin* origin = nullptr,
                           std::chrono::steady_clock::duration* made_time = nullptr);

// Stores a program linked from these sources; true when an entry was
// written. The program must have been linked with the retrievable hint
// (MarkProgramRetrievable). A failure is reported once and leaves no
// temporary file; the program itself is unaffected.
bool StoreProgram(uint32_t program, const std::string& vertex_source, const std::string& fragment_source);

// Asks the driver, before linking, to keep the program's binary.
void MarkProgramRetrievable(uint32_t program);

// Makes stored programs ahead of their use, most recently used first, until
// `deadline` (docs/design/20261103-i023-program-warmup.md); LoadCachedProgram
// hands them out. Called on the GL thread while it would otherwise wait.
// REXBOX_SHADER_WARM=<n> warms at most n entries (default 1024, 0 none).
void WarmProgramCache(std::chrono::steady_clock::time_point deadline);

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_GL_PROGRAM_CACHE_H
