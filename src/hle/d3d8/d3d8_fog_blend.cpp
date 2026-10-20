#include "d3d8_fog_blend.h"

#include <algorithm>
#include <cmath>

namespace rexbox::hle::d3d8
{

FogSettings FogFromStates(uint32_t enable, uint32_t table_mode, uint32_t range)
{
    FogSettings fog;
    if (enable == 0)
    {
        return fog;
    }
    switch (table_mode)
    {
    case 0:
        fog.mode = fogmode::kVertex;
        fog.source = fogsource::kSpecular;
        return fog;
    case 1:
        fog.mode = fogmode::kExp;
        break;
    case 2:
        fog.mode = fogmode::kExp2;
        break;
    case 3:
        fog.mode = fogmode::kLinear;
        break;
    default:
        return fog;  // not a D3DFOGMODE: off
    }
    fog.source = range != 0 ? fogsource::kRadial : fogsource::kPlanar;
    return fog;
}

float FogFactor(uint8_t mode, float c, float start, float end, float density)
{
    float f = 1.0f;
    switch (mode)
    {
    case fogmode::kVertex:
        f = c;
        break;
    case fogmode::kLinear:
        // The library's own scale for start = end is 8192 (a step at end).
        f = end != start ? (end - c) / (end - start) : (c < end ? 1.0f : 0.0f);
        break;
    case fogmode::kExp:
        f = std::exp(-density * c);
        break;
    case fogmode::kExp2:
        f = std::exp(-(density * c) * (density * c));
        break;
    default:
        return 1.0f;
    }
    return std::clamp(f, 0.0f, 1.0f);
}

std::string FogFunctionGlsl(uint8_t mode)
{
    std::string body;
    switch (mode)
    {
    case fogmode::kVertex:
        body = "c";
        break;
    case fogmode::kLinear:
        body = "u_fog_params.y != u_fog_params.x ? (u_fog_params.y - c) / (u_fog_params.y - u_fog_params.x)"
               " : (c < u_fog_params.y ? 1.0 : 0.0)";
        break;
    case fogmode::kExp:
        body = "exp(-u_fog_params.z * c)";
        break;
    case fogmode::kExp2:
        body = "exp(-(u_fog_params.z * c) * (u_fog_params.z * c))";
        break;
    default:
        return std::string();
    }
    return "uniform vec4 u_fog_params;\n"  // start, end, density
           "float rexbox_fog(float c)\n{\n    return clamp(" +
           body + ", 0.0, 1.0);\n}\n";
}

bool SkinMatrices(uint32_t mode, uint32_t* matrices, uint32_t* weights)
{
    if (mode == 0 || mode > 6)
    {
        return false;
    }
    *matrices = (mode + 3) / 2;
    *weights = (mode & 1) != 0 ? *matrices - 1 : *matrices;
    return true;
}

}  // namespace rexbox::hle::d3d8
