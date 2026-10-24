// rexbox_recomp_probe: links rexbox_recomp the way an application of a title
// with no generated code does (a tree without game data, as in CI), through
// rexbox_add_recompiled_library with an empty GEN_DIR. The register accessors
// rexbox_recomp calls then come from recomp_abi_none.c; the probe checks that
// they link and keep their values.
//
// Usage: rexbox_recomp_probe        (exit code 0 when every check passes)

#include "rexbox/recomp/recomp_abi.h"
#include "rexbox/recomp/recomp_runtime.h"
#include "rexbox/runtime/guest_cpu.h"

#include <cstdio>

namespace
{

int g_checks = 0;
int g_failures = 0;

void Check(bool ok, const char* what)
{
    ++g_checks;
    if (!ok)
    {
        ++g_failures;
        std::printf("FAIL %s\n", what);
    }
}

}  // namespace

int main()
{
    using rexbox::runtime::Gpr;

    rexbox_recomp_reset_thread(0x1000u, 0x2000u);
    Check(rexbox_recomp_get_gpr(4) == 0x1000u, "reset_thread sets esp");
    Check(rexbox_recomp_get_fs_base() == 0x2000u, "reset_thread sets the FS base");
    Check(rexbox_recomp_get_gpr(0) == 0, "reset_thread clears eax");

    rexbox::recomp::RecompExecutor executor(nullptr);
    rexbox::runtime::GuestCpu& cpu = executor.Cpu();
    cpu.Set(Gpr::kEax, 0x1234u);
    Check(cpu.Get(Gpr::kEax) == 0x1234u, "GuestCpu reads back eax");
    Check(rexbox_recomp_get_gpr(0) == 0x1234u, "GuestCpu writes the accessor's eax");

    RexboxRecompGprs gprs{};
    rexbox_recomp_get_gprs(&gprs);
    gprs.edi = 0x55u;
    rexbox_recomp_set_gprs(&gprs);
    Check(rexbox_recomp_get_gpr(7) == 0x55u, "set_gprs writes edi");

    const uint32_t lanes[4] = {1, 2, 3, 4};
    uint32_t back[4] = {};
    rexbox_recomp_set_xmm(3, lanes);
    rexbox_recomp_get_xmm(3, back);
    Check(back[0] == 1 && back[3] == 4, "xmm3 round-trips");
    Check(rexbox_recomp_last_icall() == 0, "no indirect call without generated code");

    std::printf("%d check(s), %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
