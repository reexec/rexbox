// Contract between the kernel HLE module and whatever executes guest code:
// how a kernel import is addressed once the image is loaded.
//
// The Xbox loader overwrites each kernel thunk slot with the export's address.
// rexbox writes a synthetic VA instead, one per ordinal, above the end of any
// real guest memory; calls through the slot then resolve through HleRegistry.

#ifndef REXBOX_RUNTIME_KERNEL_THUNKS_H
#define REXBOX_RUNTIME_KERNEL_THUNKS_H

#include "rexbox/runtime/guest_memory.h"

#include <cstdint>

namespace rexbox::runtime
{

constexpr GuestAddress kKernelThunkVaBase = 0xFE000000;

// Synthetic code addresses for HLE methods a title reaches through a guest
// vtable (DirectSound streams, design i054): above the kernel thunks, so the
// lifter's indirect-call check (which admits 0xFE000000 and up) lets them
// through to the registry, and not mistaken for kernel imports.
constexpr GuestAddress kHleMethodVaBase = 0xFF000000;

constexpr GuestAddress KernelThunkVa(uint32_t ordinal)
{
    return kKernelThunkVaBase + ordinal * 4;
}

constexpr bool IsKernelThunkVa(GuestAddress address)
{
    return address >= kKernelThunkVaBase && address < kHleMethodVaBase;
}

constexpr uint32_t KernelThunkOrdinal(GuestAddress address)
{
    return (address - kKernelThunkVaBase) / 4;
}

}  // namespace rexbox::runtime

#endif  // REXBOX_RUNTIME_KERNEL_THUNKS_H
