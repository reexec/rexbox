/* Definitions of everything recompiled code declares extern: its CPU state
 * and the callbacks it calls.
 *
 * The only rexbox file that includes the lifter's ABI header. It is compiled
 * with each title's generated code (rexbox_add_recompiled_library), against
 * the recomp_types.h the lifter wrote next to that code, so every definition
 * here is checked against the declaration the generated code uses. The
 * callbacks only forward to their bodies in recomp_bridge.cpp
 * (recomp_callbacks.h). */

/* The MMX register type is only declared for generated code, which also maps
 * bare register names (eax, ...) to the globals. Take the declarations and
 * drop the name mapping, which would rewrite RexboxRecompGprs's fields. */
#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
#undef eax
#undef ecx
#undef edx
#undef esp
#undef ebx
#undef esi
#undef edi

#include "rexbox/recomp/recomp_abi.h"
#include "recomp_callbacks.h"

#include <string.h>

/* Memory and code range. */
ptrdiff_t g_xbox_mem_offset;
uint32_t g_xbox_code_lo;
uint32_t g_xbox_code_hi;
int g_force_return;

/* Per-thread guest CPU state. */
RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp;
RECOMP_TLS uint32_t g_ebx, g_esi, g_edi;
RECOMP_TLS uint32_t g_ebp;
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top;
RECOMP_TLS uint16_t g_fp_control_word;
RECOMP_TLS int g_fp_cmp;
RECOMP_TLS uint16_t g_fp_cc;
RECOMP_TLS uint32_t g_mxcsr;
RECOMP_TLS uint32_t g_fs_base;
RECOMP_TLS uint32_t g_seh_ebp;
RECOMP_TLS int g_df;
/* EFLAGS image for pushfd/popfd (upstream xboxrecomp v0.15.0). */
RECOMP_TLS uint32_t g_eflags;
RECOMP_TLS RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3;
RECOMP_TLS RecompXmm g_xmm4, g_xmm5, g_xmm6, g_xmm7;
RECOMP_TLS RecompMmx g_mm0, g_mm1, g_mm2, g_mm3;
RECOMP_TLS RecompMmx g_mm4, g_mm5, g_mm6, g_mm7;

/* Indirect-call trace ring, shared by all threads like the lifter expects. */
volatile uint32_t g_icall_trace[ICALL_TRACE_SIZE];
volatile uint32_t g_icall_trace_idx;
volatile uint64_t g_icall_count;

/* RECOMP_BACK_EDGE's quantum flag (upstream xboxrecomp v0.15.0, code lifted
 * with --back-edge-yield). rexbox runs guest threads as free host threads
 * with no guest CPU lock, so nothing ever sets it. */
int32_t g_xbox_guest_cpu_preempt;

/* x87 FINIT control word: all exceptions masked, 64-bit precision, round to
 * nearest. */
#define REXBOX_X87_DEFAULT_CONTROL_WORD 0x037F

/* MXCSR reset value: all SSE exceptions masked, round to nearest. */
#define REXBOX_SSE_DEFAULT_MXCSR 0x1F80u

/* EFLAGS at thread start: bit 1 always set, IF set. */
#define REXBOX_EFLAGS_DEFAULT 0x00000202u

void rexbox_recomp_set_memory(ptrdiff_t offset, uint32_t code_lo, uint32_t code_hi)
{
    g_xbox_mem_offset = offset;
    g_xbox_code_lo = code_lo;
    g_xbox_code_hi = code_hi;
}

void rexbox_recomp_reset_thread(uint32_t esp, uint32_t fs_base)
{
    g_eax = g_ecx = g_edx = g_ebx = g_esi = g_edi = g_ebp = 0;
    g_esp = esp;
    memset(g_fp_stack, 0, sizeof(g_fp_stack));
    g_fp_top = 0;
    g_fp_control_word = REXBOX_X87_DEFAULT_CONTROL_WORD;
    g_fp_cmp = 0;
    g_fp_cc = 0;
    g_mxcsr = REXBOX_SSE_DEFAULT_MXCSR;
    g_fs_base = fs_base;
    g_seh_ebp = 0;
    g_df = 0;
    g_eflags = REXBOX_EFLAGS_DEFAULT;
}

void rexbox_recomp_get_gprs(RexboxRecompGprs* out)
{
    out->eax = g_eax;
    out->ecx = g_ecx;
    out->edx = g_edx;
    out->ebx = g_ebx;
    out->esp = g_esp;
    out->ebp = g_ebp;
    out->esi = g_esi;
    out->edi = g_edi;
}

void rexbox_recomp_set_gprs(const RexboxRecompGprs* in)
{
    g_eax = in->eax;
    g_ecx = in->ecx;
    g_edx = in->edx;
    g_ebx = in->ebx;
    g_esp = in->esp;
    g_ebp = in->ebp;
    g_esi = in->esi;
    g_edi = in->edi;
}

static RecompXmm* xmm_slot(int index)
{
    switch (index)
    {
    case 0: return &g_xmm0;
    case 1: return &g_xmm1;
    case 2: return &g_xmm2;
    case 3: return &g_xmm3;
    case 4: return &g_xmm4;
    case 5: return &g_xmm5;
    case 6: return &g_xmm6;
    default: return &g_xmm7;
    }
}

void rexbox_recomp_get_xmm(int index, uint32_t out[4])
{
    memcpy(out, xmm_slot(index), 16);
}

void rexbox_recomp_set_xmm(int index, const uint32_t in[4])
{
    memcpy(xmm_slot(index), in, 16);
}

/* A switch on the register itself rather than on its address: HLE argument
 * and result access lands here several times per call, and taking the address
 * of a thread-local through a separate function cost a measurable share of the
 * presenting thread (issue #75). */
uint32_t rexbox_recomp_get_gpr(int index)
{
    switch (index)
    {
    case 0: return g_eax;
    case 1: return g_ecx;
    case 2: return g_edx;
    case 3: return g_ebx;
    case 4: return g_esp;
    case 5: return g_ebp;
    case 6: return g_esi;
    default: return g_edi;
    }
}

void rexbox_recomp_set_gpr(int index, uint32_t value)
{
    switch (index)
    {
    case 0: g_eax = value; break;
    case 1: g_ecx = value; break;
    case 2: g_edx = value; break;
    case 3: g_ebx = value; break;
    case 4: g_esp = value; break;
    case 5: g_ebp = value; break;
    case 6: g_esi = value; break;
    default: g_edi = value; break;
    }
}

uint32_t rexbox_recomp_get_fs_base(void)
{
    return g_fs_base;
}

uint32_t rexbox_recomp_last_icall(void)
{
    if (g_icall_count == 0)
    {
        return 0;
    }
    return g_icall_trace[(g_icall_trace_idx - 1) & (ICALL_TRACE_SIZE - 1)];
}

/* The callbacks. Each one the header declares is checked against it here; a
 * declaration that exists only in some builds (recomp_abi_violation_log under
 * RECOMP_ABI_CHECK, recomp_call_profile_hit under RECOMP_CALL_PROFILE) is
 * checked in those builds. recomp_unresolved_stub_log is called by code
 * lifted with RECOMP_STUB_REPORT, which a lifter may declare in the generated
 * code instead of the header. */

recomp_func_t recomp_lookup_manual(uint32_t xbox_va)
{
    return rexbox_bridge_lookup_manual(xbox_va);
}

recomp_func_t recomp_lookup_kernel(uint32_t xbox_va)
{
    return rexbox_bridge_lookup_kernel(xbox_va);
}

void recomp_abi_violation_log(uint32_t va, uint32_t ebx0, uint32_t esi0, uint32_t edi0, uint32_t esp0)
{
    rexbox_bridge_abi_violation(va, ebx0, esi0, edi0, esp0);
}

void recomp_call_profile_hit(uint32_t va)
{
    rexbox_bridge_call_profile_hit(va);
}

void recomp_icall_fail_log(uint32_t va)
{
    rexbox_bridge_icall_fail(va);
}

void recomp_unresolved_stub_log(uint32_t va)
{
    rexbox_bridge_unresolved_stub(va);
}

void recomp_icall_not_code_log(uint32_t va)
{
    rexbox_bridge_icall_not_code(va);
}

void recomp_unimpl(const char* text, uint32_t va)
{
    rexbox_bridge_unimpl(text, va);
}

void recomp_debug_service(uint32_t service, uint32_t arg_va)
{
    rexbox_bridge_debug_service(service, arg_va);
}

void xbox_GuestCpuPreempt(uint32_t va)
{
    rexbox_bridge_guest_cpu_preempt(va);
}

void xbox_Cpuid(uint32_t leaf, uint32_t subleaf, uint32_t out[4])
{
    rexbox_bridge_cpuid(leaf, subleaf, out);
}

uint32_t xbox_PortIn(uint32_t port, int width)
{
    return rexbox_bridge_port_in(port, width);
}

void xbox_PortOut(uint32_t port, uint32_t value, int width)
{
    rexbox_bridge_port_out(port, value, width);
}

uint64_t xbox_ReadTimeStampCounter(void)
{
    return rexbox_bridge_read_tsc();
}

void recomp_trace_enter(const char* name, uint32_t va)
{
    rexbox_bridge_trace_enter(name, va);
}

void recomp_trace_exit(const char* name, uint32_t va)
{
    rexbox_bridge_trace_exit(name, va);
}

void recomp_trace_esp(const char* name, const char* tag)
{
    rexbox_bridge_trace_esp(name, tag);
}

jmp_buf* recomp_setjmp_slot(uint32_t buf_va)
{
    return (jmp_buf*)rexbox_bridge_setjmp_slot(buf_va);
}

int recomp_guest_longjmp(uint32_t buf_va, uint32_t value)
{
    return rexbox_bridge_guest_longjmp(buf_va, value);
}
