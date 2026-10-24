/* Internal to the recompiled-code adapter: the bodies of the callbacks
 * generated code calls, under rexbox's own names and signatures.
 *
 * recomp_abi.c defines the lifter's callbacks (recomp_lookup_manual, ...)
 * against the ABI header that came with the generated code, so the compiler
 * checks every signature the header declares, and forwards each call here.
 * recomp_bridge.cpp implements these on the active runtime. */
#ifndef REXBOX_RECOMP_RECOMP_CALLBACKS_H
#define REXBOX_RECOMP_RECOMP_CALLBACKS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RexboxGuestFunction)(void);

RexboxGuestFunction rexbox_bridge_lookup_manual(uint32_t va);
RexboxGuestFunction rexbox_bridge_lookup_kernel(uint32_t va);
void rexbox_bridge_abi_violation(uint32_t va, uint32_t ebx0, uint32_t esi0, uint32_t edi0, uint32_t esp0);
void rexbox_bridge_call_profile_hit(uint32_t va);
void rexbox_bridge_icall_fail(uint32_t va);
void rexbox_bridge_unresolved_stub(uint32_t va);
void rexbox_bridge_icall_not_code(uint32_t va);
void rexbox_bridge_unimpl(const char* text, uint32_t va);
void rexbox_bridge_debug_service(uint32_t service, uint32_t arg_va);
void rexbox_bridge_guest_cpu_preempt(uint32_t va);
void rexbox_bridge_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t out[4]);
uint32_t rexbox_bridge_port_in(uint32_t port, int width);
void rexbox_bridge_port_out(uint32_t port, uint32_t value, int width);
uint64_t rexbox_bridge_read_tsc(void);
void rexbox_bridge_trace_enter(const char* name, uint32_t va);
void rexbox_bridge_trace_exit(const char* name, uint32_t va);
void rexbox_bridge_trace_esp(const char* name, const char* tag);
/* A native jmp_buf for the guest jmp_buf at buf_va. */
void* rexbox_bridge_setjmp_slot(uint32_t buf_va);
int rexbox_bridge_guest_longjmp(uint32_t buf_va, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* REXBOX_RECOMP_RECOMP_CALLBACKS_H */
