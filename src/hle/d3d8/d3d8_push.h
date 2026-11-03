// Push buffers a title fills itself (D3DDevice_BeginPush/EndPush): the HLE
// has no push buffer, so BeginPush hands out guest scratch memory and EndPush
// interprets the NV2A methods written there, turning SET_BEGIN_END with
// INLINE_ARRAY vertices or ARRAY_ELEMENT16/32 indices into draws
// (docs/design/20261020-i007-push-buffer.md).

#ifndef REXBOX_HLE_D3D8_D3D8_PUSH_H
#define REXBOX_HLE_D3D8_D3D8_PUSH_H

#include "rexbox/runtime/guest_memory.h"
#include "rexbox/runtime/hle_registry.h"

#include <cstdint>

namespace rexbox::hle::d3d8
{

// Draws `bytes` of vertices at guest `data`, laid out for the current vertex
// shader (its stream 0 stride), as `primitive` (d3d8_draw.cpp).
void DrawInlineVertices(uint32_t primitive, runtime::GuestAddress data, uint32_t bytes);

// Draws `count` 16-bit indices at guest `indices` into the current stream 0
// (SetVertexShaderInput's or SetStreamSource's) as `primitive` (d3d8_draw.cpp).
void DrawArrayElements(uint32_t primitive, runtime::GuestAddress indices, uint32_t count);

// The device push buffer an LTCG title writes itself (design i051): gives the
// device object at `device` a guest buffer laid out as XDK 5849's library
// does (+0 pPut, +4 threshold, +0x24 base, +0x28 end).
void SetupDevicePushBuffer(runtime::GuestAddress device);

// Interprets what the title wrote into the device push buffer since the last
// flush. Called before every HLE call that draws, clears or presents.
void FlushDevicePushBuffer();

void RegisterPushExports(runtime::HleRegistry& registry);

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_PUSH_H
