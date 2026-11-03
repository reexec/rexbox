// D3D8LTCG register-convention variants re-framed as their public functions
// (docs/design/20261103-i050-d3d8-ltcg-variants.md).

#ifndef REXBOX_HLE_D3D8_D3D8_LTCG_H
#define REXBOX_HLE_D3D8_D3D8_LTCG_H

#include "rexbox/runtime/hle_registry.h"

namespace rexbox::hle::d3d8
{

// Registers one export per variant; call after the public exports exist (the
// variants find them by name when called).
void RegisterLtcgExports(runtime::HleRegistry& registry);

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_LTCG_H
