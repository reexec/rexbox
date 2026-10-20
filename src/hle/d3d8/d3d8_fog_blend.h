// Fog and vertex blending as the XDK library derives them from the render
// states (docs/design/20261020-i007-fog-vertex-blend.md): pure functions the
// draw path and the probe share.

#ifndef REXBOX_HLE_D3D8_D3D8_FOG_BLEND_H
#define REXBOX_HLE_D3D8_D3D8_FOG_BLEND_H

#include <cstdint>
#include <string>

namespace rexbox::hle::d3d8
{

// How the fog factor is computed per pixel from the fog coordinate c.
namespace fogmode
{
constexpr uint8_t kOff = 0;
constexpr uint8_t kVertex = 1;  // D3DFOG_NONE with fog on: c is the factor
constexpr uint8_t kLinear = 2;  // (end - c) / (end - start)
constexpr uint8_t kExp = 3;     // exp(-density * c)
constexpr uint8_t kExp2 = 4;    // exp(-(density * c)^2)
}  // namespace fogmode

// Where a fixed-function vertex's fog coordinate comes from.
namespace fogsource
{
constexpr uint8_t kPlanar = 0;    // eye-space z, sign kept (RangeFogEnable 0)
constexpr uint8_t kRadial = 1;    // eye-space distance (RangeFogEnable 1)
constexpr uint8_t kSpecular = 2;  // the specular colour's alpha (D3DFOG_NONE)
}  // namespace fogsource

struct FogSettings
{
    uint8_t mode = fogmode::kOff;
    uint8_t source = fogsource::kPlanar;
};

// D3DRS_FOGENABLE, D3DRS_FOGTABLEMODE (0 none, 1 exp, 2 exp2, 3 linear) and
// D3DRS_RANGEFOGENABLE as the XDK 4531 state flush reads them.
FogSettings FogFromStates(uint32_t enable, uint32_t table_mode, uint32_t range);

// The factor for fog coordinate `c`, clamped to 0..1 (the GLSL the shaders
// carry computes the same).
float FogFactor(uint8_t mode, float c, float start, float end, float density);

// GLSL: `float rexbox_fog(float c)` for `mode`, reading the uniform
// `u_fog_params` (start, end, density); empty for fogmode::kOff.
std::string FogFunctionGlsl(uint8_t mode);

// NV2A SET_SKIN_MODE (the D3DRS_VERTEXBLEND slot holds it): 0 off; 1, 3, 5
// two, three, four matrices with the last weight implicit (1 - the others);
// 2, 4, 6 the same with every weight given. False for 0 or a value past 6.
bool SkinMatrices(uint32_t mode, uint32_t* matrices, uint32_t* weights);

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_FOG_BLEND_H
