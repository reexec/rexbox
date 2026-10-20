#include "d3d8_render_state.h"

namespace rexbox::hle::d3d8
{

RenderStateLayout DeriveRenderStateLayout(uint32_t render_state, uint32_t deferred, uint32_t complex)
{
    RenderStateLayout layout;
    if (render_state == 0 || deferred <= render_state || complex <= deferred || (deferred - render_state) % 4 != 0 ||
        (complex - render_state) % 4 != 0)
    {
        return layout;
    }
    const uint32_t deferred_first = (deferred - render_state) / 4;
    const uint32_t complex_first = (complex - render_state) / 4;
    // The deferred block keeps its size; only the simple block may grow.
    if (deferred_first < kLogicalDeferredFirst ||
        complex_first - deferred_first != kLogicalComplexFirst - kLogicalDeferredFirst)
    {
        return layout;
    }
    layout.deferred_first = deferred_first;
    layout.complex_first = complex_first;
    return layout;
}

uint32_t RenderStateSlotCount(const RenderStateLayout& layout)
{
    return layout.complex_first + kComplexStateCount;
}

uint32_t SlotFromLogical(const RenderStateLayout& layout, uint32_t logical)
{
    if (logical < kLogicalDeferredFirst)
    {
        return logical;
    }
    if (logical < kLogicalComplexFirst)
    {
        return logical - kLogicalDeferredFirst + layout.deferred_first;
    }
    if (logical < kLogicalStateCount)
    {
        return logical - kLogicalComplexFirst + layout.complex_first;
    }
    return kNoRenderState;
}

uint32_t LogicalFromSlot(const RenderStateLayout& layout, uint32_t slot)
{
    if (slot < kLogicalDeferredFirst)
    {
        return slot;
    }
    if (slot < layout.deferred_first)
    {
        return kNoRenderState;  // a simple state the title's XDK added
    }
    if (slot < layout.complex_first)
    {
        return slot - layout.deferred_first + kLogicalDeferredFirst;
    }
    if (slot < RenderStateSlotCount(layout))
    {
        return slot - layout.complex_first + kLogicalComplexFirst;
    }
    return kNoRenderState;
}

}  // namespace rexbox::hle::d3d8
