// D3D8LTCG's register-convention variants whose parameters are the public
// function's, in its order, only passed partly in registers (design i050):
// each is re-framed as the public stdcall call and handed to its export.
// Variants that fold parameters away (Direct3D_CreateDevice_4__LTCG_eax1_ecx3)
// have their own exports next to the public one.

#include "d3d8_ltcg.h"

#include "d3d8_internal.h"

#include "rexbox/runtime/environment.h"
#include "rexbox/runtime/guest_cpu.h"

#include <array>
#include <cstdio>
#include <iterator>
#include <utility>

namespace rexbox::hle::d3d8
{

namespace
{

using runtime::Gpr;

// Where one public parameter comes from: a register, or the next stack
// argument of the variant (in order).
enum class Source : uint8_t
{
    kStack,
    kEax,
    kEcx,
    kEdx,
    kEbx,
    kEsi,
    kEdi,
};

struct Variant
{
    const char* name;    // XbSymbolDatabase's name
    const char* target;  // the public export it is re-framed for
    uint8_t stack_args;  // 4-byte arguments the variant pops (the _<n>_ in its name / 4)
    std::array<Source, 4> params;
    uint8_t param_count;
};

// The mapping assumes the LTCG signature keeps the public one (the counts of
// register and stack parameters add up to the public arity; **inferred** for
// each until a title's lifted body or a wrong result says otherwise, unlike
// CreateDevice, design i050). Names as in Pump It Up: Exceed (XDK 5849).
constexpr Variant kVariants[] = {
    {"D3DDevice_SetTransform_0__LTCG_eax1_edx2", "D3DDevice_SetTransform", 0, {Source::kEax, Source::kEdx}, 2},
    {"D3DDevice_SetStreamSource_4__LTCG_eax1_ebx2", "D3DDevice_SetStreamSource", 1,
     {Source::kEax, Source::kEbx, Source::kStack}, 3},
    {"D3DDevice_SetIndices_4__LTCG_ebx1", "D3DDevice_SetIndices", 1, {Source::kEbx, Source::kStack}, 2},
    {"D3DDevice_LightEnable_4__LTCG_eax1", "D3DDevice_LightEnable", 1, {Source::kEax, Source::kStack}, 2},
    {"D3DDevice_SetPixelShaderConstant_4__LTCG_ecx1_eax3", "D3DDevice_SetPixelShaderConstant", 1,
     {Source::kEcx, Source::kStack, Source::kEax}, 3},
    {"D3DDevice_LoadVertexShader_0__LTCG_edx1_eax2", "D3DDevice_LoadVertexShader", 0, {Source::kEdx, Source::kEax}, 2},
    {"D3DDevice_SelectVertexShader_0__LTCG_eax1_ebx2", "D3DDevice_SelectVertexShader", 0,
     {Source::kEax, Source::kEbx}, 2},
    {"D3DDevice_SetTextureStageStateNotInline_0__LTCG_ecx1_eax2_edx3", "D3DDevice_SetTextureStageStateNotInline", 0,
     {Source::kEcx, Source::kEax, Source::kEdx}, 3},
    {"D3DDevice_Swap_0__LTCG_eax1", "D3DDevice_Swap", 0, {Source::kEax}, 1},
    {"D3DDevice_SetFlickerFilter_0__LTCG_esi1", "D3DDevice_SetFlickerFilter", 0, {Source::kEsi}, 1},
    {"D3D_BlockOnResource_0__LTCG_eax1", "D3D_BlockOnResource", 0, {Source::kEax}, 1},
    // The no-wait form of SetTile: the HLE never waits either way.
    {"D3D_SetTileNoWait_0__LTCG_eax1_ecx2", "D3DDevice_SetTile", 0, {Source::kEax, Source::kEcx}, 2},
};

uint32_t Register(const runtime::GuestCpu& cpu, Source source)
{
    switch (source)
    {
    case Source::kEax:
        return cpu.Get(Gpr::kEax);
    case Source::kEcx:
        return cpu.Get(Gpr::kEcx);
    case Source::kEdx:
        return cpu.Get(Gpr::kEdx);
    case Source::kEbx:
        return cpu.Get(Gpr::kEbx);
    case Source::kEsi:
        return cpu.Get(Gpr::kEsi);
    case Source::kEdi:
        return cpu.Get(Gpr::kEdi);
    case Source::kStack:
        break;
    }
    return 0;
}

// Entered as the variant: [esp] = return address, then its stack arguments.
// Leaves the guest exactly as the variant's own `ret <n>` would, with the
// target's result in eax.
void Reframe(const Variant& variant)
{
    runtime::GuestCpu& cpu = runtime::Env().executor->Cpu();
    runtime::GuestMemory& memory = Memory();
    const uint32_t esp = cpu.Get(Gpr::kEsp);
    const uint32_t return_address = memory.Read32(esp);
    std::array<uint32_t, 4> values{};
    uint32_t next_stack = 0;
    for (uint32_t i = 0; i < variant.param_count; ++i)
    {
        values[i] = variant.params[i] == Source::kStack ? memory.Read32(esp + 4 + 4 * next_stack++)
                                                         : Register(cpu, variant.params[i]);
    }
    // The caller's esp after the variant returns; the public stdcall frame
    // [return address, P1..Pn] ends there, so the target's Return lands on it.
    const uint32_t caller_esp = esp + 4 + 4u * variant.stack_args;
    const uint32_t frame = caller_esp - 4 - 4u * variant.param_count;
    memory.Write32(frame, return_address);
    for (uint32_t i = 0; i < variant.param_count; ++i)
    {
        memory.Write32(frame + 4 + 4 * i, values[i]);
    }
    cpu.Set(Gpr::kEsp, frame);

    const runtime::HleExport* target = runtime::Env().registry->FindExport(variant.target);
    if (target == nullptr || target->thunk == nullptr)
    {
        std::fprintf(stderr, "[d3d8] %s: no export %s; returning 0\n", variant.name, variant.target);
        cpu.Set(Gpr::kEax, 0);
        cpu.Set(Gpr::kEsp, caller_esp);
        return;
    }
    target->thunk();
}

template <size_t I>
void VariantThunk()
{
    Reframe(kVariants[I]);
}

template <size_t... I>
void AddVariants(runtime::HleRegistry& registry, std::index_sequence<I...>)
{
    (registry.AddExport("d3d8", kVariants[I].name, &VariantThunk<I>), ...);
}

}  // namespace

void RegisterLtcgExports(runtime::HleRegistry& registry)
{
    AddVariants(registry, std::make_index_sequence<std::size(kVariants)>{});
}

}  // namespace rexbox::hle::d3d8
