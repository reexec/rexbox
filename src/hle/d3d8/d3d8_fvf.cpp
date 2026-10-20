#include "d3d8_fvf.h"

#include <algorithm>

namespace rexbox::hle::d3d8
{

bool DecodeFvf(uint32_t code, VertexLayout* layout)
{
    if ((code & fvf::kProgramHandle) != 0)
    {
        return false;
    }
    VertexLayout result;
    uint32_t offset = 0;
    const uint32_t position = code & fvf::kPositionMask;
    if (position == fvf::kXyz)
    {
        result.position_components = 3;
        offset = 12;
    }
    else if (position == fvf::kXyzRhw)
    {
        result.position_components = 4;
        result.pretransformed = true;
        offset = 16;
    }
    else if (position >= fvf::kXyzB1 && position <= fvf::kXyzB4)
    {
        result.position_components = 3;
        result.blend_weights = (position - fvf::kXyzRhw) / 2;
        result.blend_offset = 12;
        offset = 12 + 4 * result.blend_weights;
    }
    else
    {
        return false;
    }
    if ((code & fvf::kNormal) != 0)
    {
        result.normal = static_cast<int>(offset);
        offset += 12;
    }
    if ((code & fvf::kPointSize) != 0)
    {
        result.point_size = static_cast<int>(offset);
        offset += 4;
    }
    if ((code & fvf::kDiffuse) != 0)
    {
        result.diffuse = static_cast<int>(offset);
        offset += 4;
    }
    if ((code & fvf::kSpecular) != 0)
    {
        result.specular = static_cast<int>(offset);
        offset += 4;
    }
    result.tex_coords = (code & fvf::kTexCountMask) >> fvf::kTexCountShift;
    if (result.tex_coords > kMaxTexCoords)
    {
        return false;
    }
    static const uint32_t kComponents[4] = {2, 3, 4, 1};
    for (uint32_t i = 0; i < result.tex_coords; ++i)
    {
        result.tex_offset[i] = static_cast<int>(offset);
        result.tex_components[i] = kComponents[(code >> (fvf::kTexCoordSizeShift + 2 * i)) & 3];
        offset += 4 * result.tex_components[i];
    }
    result.stride = offset;
    *layout = result;
    return true;
}

bool LayoutFromDeclaration(const vsh::Declaration& declaration, VertexLayout* layout, std::string* error)
{
    // D3DVSDE_* register numbers.
    constexpr uint8_t kPositionRegister = 0;
    constexpr uint8_t kBlendWeightRegister = 1;
    constexpr uint8_t kNormalRegister = 2;
    constexpr uint8_t kDiffuseRegister = 3;
    constexpr uint8_t kSpecularRegister = 4;
    constexpr uint8_t kTexCoord0Register = 9;
    *layout = VertexLayout{};
    if ((declaration.streams & ~1u) != 0)
    {
        *error = "the declaration reads streams other than 0";
        return false;
    }
    layout->stride = declaration.stream_bytes[0];
    bool position = false;
    for (const vsh::VertexElement& element : declaration.elements)
    {
        const bool is_float = element.kind == vsh::ElementKind::kFloat;
        const bool is_color = element.kind == vsh::ElementKind::kD3dColor;
        if (element.reg == kPositionRegister)
        {
            if (!is_float || element.components < 3)
            {
                *error = "position type " + std::to_string(element.type) + " is not a float3/float4";
                return false;
            }
            layout->position = element.offset;
            layout->position_components = 3;
            position = true;
        }
        else if (element.reg == kBlendWeightRegister)
        {
            // XDK 4531 titles store weights as PBYTE (Crazy Taxi 3: 0x44).
            if (!is_float && element.kind != vsh::ElementKind::kNormByte &&
                element.kind != vsh::ElementKind::kNormShort)
            {
                *error = "blend weight type " + std::to_string(element.type) + " is not float, PBYTE or NORMSHORT";
                return false;
            }
            layout->blend_offset = element.offset;
            layout->blend_weights = element.components;
            layout->blend_kind = element.kind;
        }
        else if (element.reg == kNormalRegister)
        {
            const bool packed = element.kind == vsh::ElementKind::kNormPacked3;
            if (!packed && !(is_float && element.components == 3))
            {
                *error = "normal type " + std::to_string(element.type) + " is not float3 or NORMPACKED3";
                return false;
            }
            layout->normal = element.offset;
            layout->normal_packed = packed;
        }
        else if (element.reg == kDiffuseRegister || element.reg == kSpecularRegister)
        {
            if (!is_color)
            {
                *error = "color type " + std::to_string(element.type) + " is not D3DCOLOR";
                return false;
            }
            (element.reg == kDiffuseRegister ? layout->diffuse : layout->specular) = element.offset;
        }
        else if (element.reg >= kTexCoord0Register && element.reg < kTexCoord0Register + kMaxTexCoords)
        {
            if (!is_float)
            {
                *error = "texture coordinate type " + std::to_string(element.type) + " is not float";
                return false;
            }
            const uint32_t set = element.reg - kTexCoord0Register;
            layout->tex_offset[set] = element.offset;
            layout->tex_components[set] = element.components;
            layout->tex_coords = std::max<uint32_t>(layout->tex_coords, set + 1);
        }
    }
    // Sets the declaration skips stay at offset kAbsent with 2 components.
    for (uint32_t set = 0; set < layout->tex_coords; ++set)
    {
        if (layout->tex_components[set] == 0)
        {
            layout->tex_components[set] = 2;
        }
    }
    if (!position)
    {
        *error = "no position";
        return false;
    }
    return true;
}

}  // namespace rexbox::hle::d3d8
