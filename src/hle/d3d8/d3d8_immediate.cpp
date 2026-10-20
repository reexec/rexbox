#include "d3d8_immediate.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace rexbox::hle::d3d8
{

namespace
{

void PutFloats(uint8_t* out, const float* values, uint32_t count)
{
    std::memcpy(out, values, 4u * count);
}

uint8_t UnitByte(float value)
{
    return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

// A D3DCOLOR from (r, g, b, a) in 0..1: bytes B, G, R, A.
void PutColor(uint8_t* out, const float* rgba)
{
    out[0] = UnitByte(rgba[2]);
    out[1] = UnitByte(rgba[1]);
    out[2] = UnitByte(rgba[0]);
    out[3] = UnitByte(rgba[3]);
}

void PutShort(uint8_t* out, float value)
{
    const int16_t v = static_cast<int16_t>(std::lround(std::clamp(value, -32768.0f, 32767.0f)));
    std::memcpy(out, &v, 2);
}

// A D3DVSDT_NORMPACKED3 DWORD: signed X:11 Y:11 Z:10 from -1..1.
void PutNormPacked3(uint8_t* out, const float* value)
{
    const auto field = [](float v, float scale, uint32_t bits) {
        const float clamped = std::min(1.0f, std::max(-1.0f, v));
        return static_cast<uint32_t>(static_cast<int32_t>(std::lround(clamped * scale))) & ((1u << bits) - 1u);
    };
    const uint32_t packed =
        field(value[0], 1023.0f, 11) | (field(value[1], 1023.0f, 11) << 11) | (field(value[2], 511.0f, 10) << 22);
    std::memcpy(out, &packed, 4);
}

}  // namespace

ImmediateBatch::ImmediateBatch()
{
    for (auto& reg : current_.reg)
    {
        reg[0] = 0.0f;
        reg[1] = 0.0f;
        reg[2] = 0.0f;
        reg[3] = 1.0f;
    }
}

void ImmediateBatch::Begin(uint32_t primitive)
{
    active_ = true;
    primitive_ = primitive;
    vertices_.clear();
}

bool ImmediateBatch::Set(uint32_t reg, float x, float y, float z, float w)
{
    const bool complete = reg == vsde::kVertex;
    if (complete)
    {
        reg = vsde::kPosition;
    }
    if (reg >= kImmediateRegisters)
    {
        return false;
    }
    float* value = current_.reg[reg];
    value[0] = x;
    value[1] = y;
    value[2] = z;
    value[3] = w;
    if (complete && active_)
    {
        vertices_.push_back(current_);
    }
    return true;
}

std::vector<uint8_t> PackFvfVertices(const VertexLayout& layout, const std::vector<ImmediateVertex>& vertices)
{
    std::vector<uint8_t> bytes(static_cast<size_t>(layout.stride) * vertices.size());
    uint8_t* out = bytes.data();
    for (const ImmediateVertex& vertex : vertices)
    {
        PutFloats(out + layout.position, vertex.reg[vsde::kPosition], layout.position_components);
        if (layout.blend_weights != 0)
        {
            uint8_t* at = out + (layout.blend_offset != kAbsent ? layout.blend_offset : 4 * layout.position_components);
            const float* value = vertex.reg[vsde::kBlendWeight];
            for (uint32_t i = 0; i < layout.blend_weights; ++i)
            {
                switch (layout.blend_kind)
                {
                case vsh::ElementKind::kNormByte:
                    at[i] = UnitByte(value[i]);
                    break;
                case vsh::ElementKind::kNormShort:
                    PutShort(at + 2 * i, value[i] * 32767.0f);
                    break;
                default:
                    PutFloats(at + 4 * i, value + i, 1);
                    break;
                }
            }
        }
        if (layout.normal != kAbsent && layout.normal_packed)
        {
            PutNormPacked3(out + layout.normal, vertex.reg[vsde::kNormal]);
        }
        else if (layout.normal != kAbsent)
        {
            PutFloats(out + layout.normal, vertex.reg[vsde::kNormal], 3);
        }
        if (layout.point_size != kAbsent)
        {
            PutFloats(out + layout.point_size, vertex.reg[vsde::kPointSize], 1);
        }
        if (layout.diffuse != kAbsent)
        {
            PutColor(out + layout.diffuse, vertex.reg[vsde::kDiffuse]);
        }
        if (layout.specular != kAbsent)
        {
            PutColor(out + layout.specular, vertex.reg[vsde::kSpecular]);
        }
        for (uint32_t t = 0; t < layout.tex_coords; ++t)
        {
            if (layout.tex_offset[t] != kAbsent)
            {
                PutFloats(out + layout.tex_offset[t], vertex.reg[vsde::kTexCoord0 + t], layout.tex_components[t]);
            }
        }
        out += layout.stride;
    }
    return bytes;
}

std::vector<uint8_t> PackDeclarationVertices(const vsh::Declaration& declaration, uint32_t stride,
                                             const std::vector<ImmediateVertex>& vertices)
{
    std::vector<uint8_t> bytes(static_cast<size_t>(stride) * vertices.size());
    for (size_t v = 0; v < vertices.size(); ++v)
    {
        uint8_t* out = bytes.data() + v * stride;
        for (const vsh::VertexElement& element : declaration.elements)
        {
            if (element.stream != 0 || element.reg >= kImmediateRegisters)
            {
                continue;
            }
            const float* value = vertices[v].reg[element.reg];
            uint8_t* at = out + element.offset;
            const uint32_t count = std::min<uint32_t>(element.components, 4);
            switch (element.kind)
            {
            case vsh::ElementKind::kFloat:
                PutFloats(at, value, count);
                break;
            case vsh::ElementKind::kNormShort:
                for (uint32_t i = 0; i < count; ++i)
                {
                    PutShort(at + 2 * i, value[i] * 32767.0f);
                }
                break;
            case vsh::ElementKind::kShort:
                for (uint32_t i = 0; i < count; ++i)
                {
                    PutShort(at + 2 * i, value[i]);
                }
                break;
            case vsh::ElementKind::kNormByte:
                for (uint32_t i = 0; i < count; ++i)
                {
                    at[i] = UnitByte(value[i]);
                }
                break;
            case vsh::ElementKind::kD3dColor:
                PutColor(at, value);
                break;
            case vsh::ElementKind::kNormPacked3:
                PutNormPacked3(at, value);
                break;
            }
        }
    }
    return bytes;
}

}  // namespace rexbox::hle::d3d8
