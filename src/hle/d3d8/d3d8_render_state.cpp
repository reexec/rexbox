#include "d3d8_render_state.h"

#include <array>

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

namespace
{

// The NV2A method of each state 0..81, in logical order (the XDK 4531
// library's table at 0x0012A590 in Crazy Taxi 3; 0..56 are the pixel-shader
// register-combiner states).
constexpr uint16_t kStateMethods[kLogicalDeferredFirst] = {
    0x260, 0x264, 0x268, 0x26C, 0x270, 0x274, 0x278, 0x27C, 0x288, 0x28C,   // 0..9
    0xA60, 0xA64, 0xA68, 0xA6C, 0xA70, 0xA74, 0xA78, 0xA7C, 0xA80, 0xA84,   // 10..19
    0xA88, 0xA8C, 0xA90, 0xA94, 0xA98, 0xA9C, 0xAA0, 0xAA4, 0xAA8, 0xAAC,   // 20..29
    0xAB0, 0xAB4, 0xAB8, 0xABC, 0xAC0, 0xAC4, 0xAC8, 0xACC, 0xAD0, 0xAD4,   // 30..39
    0xAD8, 0xADC, 0x17F8, 0x1E20, 0x1E24, 0x1E40, 0x1E44, 0x1E48, 0x1E4C, 0x1E50,  // 40..49
    0x1E54, 0x1E58, 0x1E5C, 0x1E60, 0x1D90, 0x1E74, 0x1E78,  // 50..56
    0x354, 0x33C, 0x304, 0x300, 0x340, 0x344, 0x348, 0x35C, 0x310, 0x37C,  // 57..66
    0x358, 0x374, 0x378, 0x364, 0x368, 0x36C, 0x360, 0x350, 0x34C, 0x9F8,  // 67..76
    0x384, 0x388, 0x330, 0x334, 0x338,  // 77..81
};

}  // namespace

uint32_t RenderStateFromNv2aMethod(uint32_t method)
{
    // Methods are below 0x2000 and 4-aligned: one byte per method, 0xFF none.
    static const std::array<uint8_t, 0x800> states = [] {
        std::array<uint8_t, 0x800> table{};
        table.fill(0xFF);
        for (uint32_t state = 0; state < kLogicalDeferredFirst; ++state)
        {
            table[kStateMethods[state] >> 2] = static_cast<uint8_t>(state);
        }
        return table;
    }();
    const uint32_t index = (method & 0x1FFC) >> 2;
    return (method & ~0x1FFCu) == 0 && states[index] != 0xFF ? states[index] : kNoRenderState;
}

}  // namespace rexbox::hle::d3d8
