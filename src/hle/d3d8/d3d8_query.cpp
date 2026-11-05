// Device queries and small device calls an XDK 5849 title reaches directly
// (issue #56, docs/design/20261103-i056-title-reached-exports.md): the
// answers come from the HLE's own state, since the HLE device object holds
// none of the library's fields.

#include "d3d8_globals.h"
#include "d3d8_internal.h"
#include "d3d8_render_state.h"
#include "d3d8_resource.h"
#include "d3d8_state.h"
#include "d3d8_vblank.h"

#include <cstring>

namespace rexbox::hle::d3d8
{

namespace
{

constexpr uint32_t kFieldProgressive = 3;  // D3DFIELD_PROGRESSIVE (inferred)

uint32_t FloatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// Accepted only: a value the HLE does not render.
void AcceptOne()
{
    HleCall call;
    call.Return(kD3dOk, 1);
}

// void D3DDevice_GetViewport(D3DVIEWPORT8* pViewport)
void D3DDevice_GetViewport()
{
    HleCall call;
    const GuestAddress out = call.Arg(0);
    if (out != 0)
    {
        const Viewport& vp = State().viewport;
        runtime::GuestMemory& memory = Memory();
        memory.Write32(out + layout::kViewportX, vp.x);
        memory.Write32(out + layout::kViewportY, vp.y);
        memory.Write32(out + layout::kViewportWidth, vp.width);
        memory.Write32(out + layout::kViewportHeight, vp.height);
        memory.Write32(out + layout::kViewportMinZ, FloatBits(vp.min_z));
        memory.Write32(out + layout::kViewportMaxZ, FloatBits(vp.max_z));
    }
    call.Return(kD3dOk, 1);
}

// void D3DDevice_GetDisplayMode(D3DDISPLAYMODE* pMode): {Width, Height,
// RefreshRate, Flags, Format}
void D3DDevice_GetDisplayMode()
{
    HleCall call;
    const GuestAddress out = call.Arg(0);
    if (out != 0)
    {
        const PresentParameters& pp = State().present;
        runtime::GuestMemory& memory = Memory();
        memory.Write32(out + 0x00, pp.back_buffer_width);
        memory.Write32(out + 0x04, pp.back_buffer_height);
        memory.Write32(out + 0x08, pp.refresh_rate != 0 ? pp.refresh_rate : 60);
        memory.Write32(out + 0x0C, pp.flags);
        memory.Write32(out + 0x10, pp.back_buffer_format);
    }
    call.Return(kD3dOk, 1);
}

// HRESULT D3DDevice_GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS*):
// {AdapterOrdinal, DeviceType, hFocusWindow, BehaviorFlags}
void D3DDevice_GetCreationParameters()
{
    HleCall call;
    const GuestAddress out = call.Arg(0);
    if (out != 0)
    {
        runtime::GuestMemory& memory = Memory();
        memory.Write32(out + 0x00, 0);
        memory.Write32(out + 0x04, kDevTypeHal);
        memory.Write32(out + 0x08, 0);
        memory.Write32(out + 0x0C, State().behavior_flags);
    }
    call.Return(kD3dOk, 1);
}

// D3DSurface* D3DDevice_GetRenderTarget2(void): the current target, with a
// reference the caller releases.
void D3DDevice_GetRenderTarget2()
{
    HleCall call;
    const GuestAddress target = State().render_target;
    if (target != 0)
    {
        AddResourceRef(target);
    }
    call.Return(target, 0);
}

// D3DSurface* D3DDevice_GetDepthStencilSurface2(void)
void D3DDevice_GetDepthStencilSurface2()
{
    HleCall call;
    const GuestAddress depth = State().depth_stencil;
    if (depth != 0)
    {
        AddResourceRef(depth);
    }
    call.Return(depth, 0);
}

// void D3DDevice_GetVertexShader(DWORD* pHandle), GetPixelShader,
// GetShaderConstantMode: the values the setters stored.
void D3DDevice_GetVertexShader()
{
    HleCall call;
    if (call.Arg(0) != 0)
    {
        Memory().Write32(call.Arg(0), Pipeline().vertex_shader);
    }
    call.Return(kD3dOk, 1);
}

void D3DDevice_GetPixelShader()
{
    HleCall call;
    if (call.Arg(0) != 0)
    {
        Memory().Write32(call.Arg(0), Pipeline().pixel_shader);
    }
    call.Return(kD3dOk, 1);
}

void D3DDevice_GetShaderConstantMode()
{
    HleCall call;
    if (call.Arg(0) != 0)
    {
        Memory().Write32(call.Arg(0), Pipeline().shader_constant_mode);
    }
    call.Return(kD3dOk, 1);
}

// void D3DDevice_GetScissors(DWORD* pCount, BOOL* pExclusive, D3DRECT* pRects):
// the HLE does not keep scissors (SetScissors is accepted only), so none.
void D3DDevice_GetScissors()
{
    HleCall call;
    if (call.Arg(0) != 0)
    {
        Memory().Write32(call.Arg(0), 0);
    }
    if (call.Arg(1) != 0)
    {
        Memory().Write32(call.Arg(1), 0);
    }
    call.Return(kD3dOk, 3);
}

// void D3DDevice_GetDisplayFieldStatus(D3DFIELD_STATUS* pFieldStatus):
// {Field, VBlankCount}; the HLE's display is progressive.
void D3DDevice_GetDisplayFieldStatus()
{
    HleCall call;
    const GuestAddress out = call.Arg(0);
    if (out != 0)
    {
        Memory().Write32(out + 0, kFieldProgressive);
        Memory().Write32(out + 4, static_cast<uint32_t>(VerticalBlankCount()));
    }
    call.Return(kD3dOk, 1);
}

// HRESULT D3DDevice_PersistDisplay(void): keeps the picture across a reboot
// into another XBE; nothing to keep here.
void D3DDevice_PersistDisplay()
{
    HleCall call;
    call.Return(kD3dOk, 0);
}

// void D3D_DestroyResource(D3DResource* = edi) (`_0__LTCG_edi1`): the
// library frees a resource whose last reference went, through the title's
// allocator (XMemFree of the data and the header). The HLE drops its host
// state and runs that original body (issue #67); before, it freed nothing and
// a title that recycles textures ran out of guest memory.
void D3D_DestroyResource_0__LTCG_edi1()
{
    HleCall call;
    if (DestroyResourceHostSide(call.Edi()) && runtime::RunLiftedOriginal())
    {
        return;  // the original returned to the caller
    }
    call.Return(call.Eax(), 0);
}

// void D3D_UpdateProjectionViewportTransform(void): the library's composite
// of the projection and viewport; the HLE derives it at draw time.
void D3D_UpdateProjectionViewportTransform()
{
    HleCall call;
    call.Return(call.Eax(), 0);
}

// void D3DDevice_SetRenderState_SampleAlpha(DWORD Value): XDK 5849's complex
// state, stored in its slot (issue #60; alpha to coverage is not rendered).
void D3DDevice_SetRenderState_SampleAlpha()
{
    HleCall call;
    SetRenderStateValue(rs::kSampleAlpha, call.Arg(0));
    call.Return(kD3dOk, 1);
}

}  // namespace

void RegisterQueryExports(runtime::HleRegistry& registry)
{
    registry.AddExport("d3d8", "D3DDevice_GetViewport", &D3DDevice_GetViewport);
    registry.AddExport("d3d8", "D3DDevice_GetDisplayMode", &D3DDevice_GetDisplayMode);
    registry.AddExport("d3d8", "D3DDevice_GetCreationParameters", &D3DDevice_GetCreationParameters);
    registry.AddExport("d3d8", "D3DDevice_GetRenderTarget2", &D3DDevice_GetRenderTarget2);
    registry.AddExport("d3d8", "D3DDevice_GetDepthStencilSurface2", &D3DDevice_GetDepthStencilSurface2);
    registry.AddExport("d3d8", "D3DDevice_GetVertexShader", &D3DDevice_GetVertexShader);
    registry.AddExport("d3d8", "D3DDevice_GetPixelShader", &D3DDevice_GetPixelShader);
    registry.AddExport("d3d8", "D3DDevice_GetShaderConstantMode", &D3DDevice_GetShaderConstantMode);
    registry.AddExport("d3d8", "D3DDevice_GetScissors", &D3DDevice_GetScissors);
    registry.AddExport("d3d8", "D3DDevice_GetDisplayFieldStatus", &D3DDevice_GetDisplayFieldStatus);
    registry.AddExport("d3d8", "D3DDevice_PersistDisplay", &D3DDevice_PersistDisplay);
    registry.AddExport("d3d8", "D3D_DestroyResource_0__LTCG_edi1", &D3D_DestroyResource_0__LTCG_edi1);
    registry.AddExport("d3d8", "D3D_UpdateProjectionViewportTransform", &D3D_UpdateProjectionViewportTransform);
    // Accepted only: the overlay (a video plane the HLE does not composite).
    registry.AddExport("d3d8", "D3DDevice_EnableOverlay", &AcceptOne);
    registry.AddExport("d3d8", "D3DDevice_SetRenderState_SampleAlpha", &D3DDevice_SetRenderState_SampleAlpha);
}

}  // namespace rexbox::hle::d3d8
