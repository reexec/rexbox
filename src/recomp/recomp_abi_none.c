/* rexbox's side of the lifter ABI for a title with no generated code yet (a
 * tree without game data, such as CI).
 *
 * recomp_abi.c, which defines the register accessors against the lifter's
 * header, is compiled only with generated code. Without it the application
 * still links rexbox_recomp, which calls the accessors, so this file defines
 * them on a plain per-thread register file: no guest code exists to run, and
 * the values only have to be consistent. rexbox_add_recompiled_library links
 * it in place of recomp_abi.c when GEN_DIR holds no generated code. */

#include "rexbox/recomp/recomp_abi.h"

#include <string.h>

#if defined(_MSC_VER)
#define REXBOX_NONE_TLS __declspec(thread)
#else
#define REXBOX_NONE_TLS _Thread_local
#endif

static REXBOX_NONE_TLS uint32_t t_gpr[8];
static REXBOX_NONE_TLS uint32_t t_xmm[8][4];
static REXBOX_NONE_TLS uint32_t t_fs_base;

void rexbox_recomp_set_memory(ptrdiff_t offset, uint32_t code_lo, uint32_t code_hi)
{
    (void)offset;
    (void)code_lo;
    (void)code_hi;
}

void rexbox_recomp_reset_thread(uint32_t esp, uint32_t fs_base)
{
    memset(t_gpr, 0, sizeof(t_gpr));
    memset(t_xmm, 0, sizeof(t_xmm));
    t_gpr[4] = esp;
    t_fs_base = fs_base;
}

void rexbox_recomp_get_gprs(RexboxRecompGprs* out)
{
    out->eax = t_gpr[0];
    out->ecx = t_gpr[1];
    out->edx = t_gpr[2];
    out->ebx = t_gpr[3];
    out->esp = t_gpr[4];
    out->ebp = t_gpr[5];
    out->esi = t_gpr[6];
    out->edi = t_gpr[7];
}

void rexbox_recomp_set_gprs(const RexboxRecompGprs* in)
{
    t_gpr[0] = in->eax;
    t_gpr[1] = in->ecx;
    t_gpr[2] = in->edx;
    t_gpr[3] = in->ebx;
    t_gpr[4] = in->esp;
    t_gpr[5] = in->ebp;
    t_gpr[6] = in->esi;
    t_gpr[7] = in->edi;
}

uint32_t rexbox_recomp_get_gpr(int index)
{
    return t_gpr[index & 7];
}

void rexbox_recomp_set_gpr(int index, uint32_t value)
{
    t_gpr[index & 7] = value;
}

uint32_t rexbox_recomp_get_fs_base(void)
{
    return t_fs_base;
}

void rexbox_recomp_get_xmm(int index, uint32_t out[4])
{
    memcpy(out, t_xmm[index & 7], 16);
}

void rexbox_recomp_set_xmm(int index, const uint32_t in[4])
{
    memcpy(t_xmm[index & 7], in, 16);
}

uint32_t rexbox_recomp_last_icall(void)
{
    return 0;
}
