#include "rexbox/hle/xapi/xapi_hle.h"

#include "rexbox/runtime/hle_call.h"
#include "rexbox/runtime/hle_registry.h"

#include <cstdint>
#include <cstdio>

namespace rexbox::hle::xapi
{

namespace
{

using runtime::HleCall;

// The host offers no Xbox network (issue #53, docs/design/20261103-i053-xnet-offline.md):
// the lifted XNET library would program the console's NIC through its MMIO
// window. Titles that call these take their offline path.
constexpr uint32_t kWsaENetDown = 10050;

// INT XNetStartup(const XNetStartupParams* pxnsp)
void XNetStartup()
{
    HleCall call;
    static bool reported = false;
    if (!reported)
    {
        reported = true;
        std::fprintf(stderr, "[xnet] XNetStartup: no network on this host (WSAENETDOWN); offline\n");
    }
    call.Return(kWsaENetDown, 1);
}

// DWORD XNetGetEthernetLinkStatus(void): 0, no cable connected.
void XNetGetEthernetLinkStatus()
{
    HleCall().Return(0, 0);
}

}  // namespace

void RegisterXapiHle(runtime::HleRegistry& registry)
{
    registry.AddExport("xapi", "XNetStartup", &XNetStartup);
    registry.AddExport("xapi", "XNetGetEthernetLinkStatus", &XNetGetEthernetLinkStatus);
}

}  // namespace rexbox::hle::xapi
