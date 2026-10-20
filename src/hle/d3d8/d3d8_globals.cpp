#include "d3d8_globals.h"

#include "d3d8_internal.h"
#include "d3d8_render_state.h"

#include <atomic>

namespace rexbox::hle::d3d8
{

namespace
{

// XbSymbolDatabase names (gen/<title>/xdk_symbols.json).
constexpr const char* kDeviceVariable = "D3D_g_pDevice";
constexpr const char* kStreamVariable = "D3D_g_Stream";
constexpr const char* kRenderStateVariable = "D3D_g_RenderState";
constexpr const char* kTextureStateVariable = "D3D_g_DeferredTextureState";
constexpr const char* kDeferredRenderStateVariable = "D3D_g_DeferredRenderState";
constexpr const char* kComplexRenderStateVariable = "D3D_g_ComplexRenderState";

struct Addresses
{
    GuestAddress device = 0;
    GuestAddress stream = 0;
    GuestAddress render_state = 0;
    GuestAddress texture_state = 0;
    RenderStateLayout layout;  // from the bound block variables, else 4134's
};

// Looked up on first use after the title bound its variables; kept once the
// render state is found.
const Addresses& Resolve()
{
    static Addresses addresses;
    static std::atomic<bool> resolved{false};
    if (!resolved.load(std::memory_order_acquire))
    {
        runtime::HleRegistry* registry = runtime::Env().registry;
        if (registry != nullptr)
        {
            Addresses found;
            found.device = registry->FindVariable(kDeviceVariable);
            found.stream = registry->FindVariable(kStreamVariable);
            found.render_state = registry->FindVariable(kRenderStateVariable);
            found.texture_state = registry->FindVariable(kTextureStateVariable);
            found.layout = DeriveRenderStateLayout(found.render_state,
                                                   registry->FindVariable(kDeferredRenderStateVariable),
                                                   registry->FindVariable(kComplexRenderStateVariable));
            // The profile binds every identified variable at once, so a bound
            // render state means binding is over; a variable the title's
            // library build does not identify (XDK 4531 has no D3D_g_Stream)
            // stays 0 and its accessors read 0 and drop writes.
            if (found.render_state != 0)
            {
                addresses = found;
                resolved.store(true, std::memory_order_release);
            }
        }
    }
    return addresses;
}

}  // namespace

bool LibraryGlobalsAvailable()
{
    return Resolve().device != 0;
}

GuestAddress DeviceObject()
{
    const GuestAddress slot = Resolve().device;
    return slot != 0 ? Memory().Read32(slot) : State().device;
}

void SetDeviceObject(GuestAddress device)
{
    if (const GuestAddress slot = Resolve().device)
    {
        Memory().Write32(slot, device);
    }
}

const RenderStateLayout& CurrentRenderStateLayout()
{
    return Resolve().layout;
}

uint32_t RenderStateSlotValue(uint32_t slot)
{
    const Addresses& addresses = Resolve();
    return addresses.render_state != 0 && slot < RenderStateSlotCount(addresses.layout)
               ? Memory().Read32(addresses.render_state + slot * 4)
               : 0;
}

void SetRenderStateSlot(uint32_t slot, uint32_t value)
{
    const Addresses& addresses = Resolve();
    if (addresses.render_state != 0 && slot < RenderStateSlotCount(addresses.layout))
    {
        Memory().Write32(addresses.render_state + slot * 4, value);
    }
}

uint32_t RenderStateValue(uint32_t state)
{
    const uint32_t slot = SlotFromLogical(Resolve().layout, state);
    return slot != kNoRenderState ? RenderStateSlotValue(slot) : 0;
}

void SetRenderStateValue(uint32_t state, uint32_t value)
{
    const uint32_t slot = SlotFromLogical(Resolve().layout, state);
    if (slot != kNoRenderState)
    {
        SetRenderStateSlot(slot, value);
    }
}

uint32_t TextureStateValue(uint32_t stage, uint32_t slot)
{
    const GuestAddress base = Resolve().texture_state;
    if (base == 0 || stage >= globals::kTextureStages || slot >= globals::kTextureStateSlots)
    {
        return 0;
    }
    return Memory().Read32(base + (stage * globals::kTextureStateSlots + slot) * 4);
}

void SetTextureStateValue(uint32_t stage, uint32_t slot, uint32_t value)
{
    const GuestAddress base = Resolve().texture_state;
    if (base != 0 && stage < globals::kTextureStages && slot < globals::kTextureStateSlots)
    {
        Memory().Write32(base + (stage * globals::kTextureStateSlots + slot) * 4, value);
    }
}

GuestAddress StreamVertexBuffer(uint32_t stream)
{
    const GuestAddress base = Resolve().stream;
    return base != 0 && stream < globals::kStreamCount
               ? Memory().Read32(base + stream * globals::kStreamEntrySize + globals::kStreamVertexBuffer)
               : 0;
}

uint32_t StreamStride(uint32_t stream)
{
    const GuestAddress base = Resolve().stream;
    return base != 0 && stream < globals::kStreamCount
               ? Memory().Read32(base + stream * globals::kStreamEntrySize + globals::kStreamStride)
               : 0;
}

void SetStream(uint32_t stream, GuestAddress vertex_buffer, uint32_t stride)
{
    const GuestAddress base = Resolve().stream;
    if (base != 0 && stream < globals::kStreamCount)
    {
        const GuestAddress entry = base + stream * globals::kStreamEntrySize;
        Memory().Write32(entry + globals::kStreamStride, stride);
        Memory().Write32(entry + globals::kStreamVertexBuffer, vertex_buffer);
    }
}

GuestAddress IndexData()
{
    const GuestAddress base = Resolve().stream;
    return base != 0 ? Memory().Read32(base + globals::kIndexDataOffset) : 0;
}

void SetIndexData(GuestAddress data)
{
    if (const GuestAddress base = Resolve().stream)
    {
        Memory().Write32(base + globals::kIndexDataOffset, data);
    }
}

void OrDirtyFlags(uint32_t bits)
{
    if (const GuestAddress base = Resolve().stream)
    {
        const GuestAddress flags = base + globals::kDirtyFlagsOffset;
        Memory().Write32(flags, Memory().Read32(flags) | bits);
    }
}

uint32_t DevicePushFence()
{
    const GuestAddress device = DeviceObject();
    return device != 0 ? Memory().Read32(device + globals::kDevicePushFence) : 0;
}

}  // namespace rexbox::hle::d3d8
