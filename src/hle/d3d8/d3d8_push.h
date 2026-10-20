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

void RegisterPushExports(runtime::HleRegistry& registry);

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_PUSH_H
