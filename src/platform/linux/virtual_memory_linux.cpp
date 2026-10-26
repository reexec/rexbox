// Virtual memory contract on Linux: anonymous mmap and mprotect. There is no
// written-page tracking: blocks asked for it are allocated untracked and
// TakeWrittenPages reports failure, so callers treat every page as written.
// The kernel's soft-dirty bits were used once, but they can only be cleared
// for the whole process (/proc/self/clear_refs), which costs milliseconds per
// query with a GL driver's mappings and drops the record of ranges not queried
// yet (docs/design/20261027-i036-linux-write-tracking.md).

#include "rexbox/platform/virtual_memory.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>

namespace rexbox::platform
{

namespace
{

int ToProtection(PageAccess access)
{
    switch (access)
    {
    case PageAccess::kNone:
        return PROT_NONE;
    case PageAccess::kRead:
        return PROT_READ;
    case PageAccess::kReadWrite:
        return PROT_READ | PROT_WRITE;
    case PageAccess::kReadExecute:
        return PROT_READ | PROT_EXEC;
    case PageAccess::kReadWriteExecute:
        return PROT_READ | PROT_WRITE | PROT_EXEC;
    }
    return PROT_NONE;
}

}  // namespace

void* AllocateVirtualMemory(size_t size, void* preferred_address, bool require_preferred, bool /*track_writes*/)
{
    const int protection = PROT_READ | PROT_WRITE;
    void* block = MAP_FAILED;
    if (preferred_address != nullptr)
    {
        block = mmap(preferred_address, size, protection, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        // An old kernel without MAP_FIXED_NOREPLACE treats it as a hint and
        // may map elsewhere; that is not the preferred address.
        if (block != MAP_FAILED && block != preferred_address)
        {
            munmap(block, size);
            block = MAP_FAILED;
        }
        if (block == MAP_FAILED && require_preferred)
        {
            return nullptr;
        }
    }
    if (block == MAP_FAILED)
    {
        block = mmap(nullptr, size, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    if (block == MAP_FAILED)
    {
        return nullptr;
    }
    return block;
}

void FreeVirtualMemory(void* address, size_t size)
{
    if (address == nullptr)
    {
        return;
    }
    munmap(address, size);
}

bool TakeWrittenPages(void* /*address*/, size_t /*size*/, void** /*pages*/, size_t /*capacity*/, size_t* /*count*/)
{
    return false;
}

bool ProtectVirtualMemory(void* address, size_t size, PageAccess access)
{
    return mprotect(address, size, ToProtection(access)) == 0;
}

size_t PageSize()
{
    static const size_t page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    return page_size;
}

namespace
{

// Maps nothing at [address, address + size) to claim it, or fails when any
// of it is taken (MAP_FIXED_NOREPLACE, Linux 4.17+).
bool Claim(uintptr_t address, size_t size)
{
    void* placed = mmap(reinterpret_cast<void*>(address), size, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
    if (placed == MAP_FAILED)
    {
        return false;
    }
    if (placed != reinterpret_cast<void*>(address))
    {
        munmap(placed, size);  // an old kernel took the address as a hint
        return false;
    }
    return true;
}

}  // namespace

bool AllocateGuestSpace(size_t ram, size_t extra, const uintptr_t* aliases, size_t alias_count, void** base)
{
    const int fd = memfd_create("rexbox-guest-ram", MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, static_cast<off_t>(ram)) != 0)
    {
        if (fd >= 0)
        {
            close(fd);
        }
        return false;
    }
    // Candidate bases step through the address space; each candidate claims
    // its ranges first (so nothing else lands there), then the claims are
    // replaced by the views.
    const uintptr_t start = sizeof(void*) == 4 ? 0x10000000u : (uintptr_t{1} << 36);
    const uintptr_t step = sizeof(void*) == 4 ? 0x01000000u : (uintptr_t{1} << 32);
    for (uintptr_t candidate = start; candidate != 0 && candidate >= start; candidate += step)
    {
        if (!Claim(candidate, ram + extra))
        {
            continue;
        }
        size_t claimed = 0;
        for (; claimed < alias_count; ++claimed)
        {
            if (!Claim(candidate + aliases[claimed], ram))
            {
                break;
            }
        }
        if (claimed != alias_count)
        {
            for (size_t i = 0; i < claimed; ++i)
            {
                munmap(reinterpret_cast<void*>(candidate + aliases[i]), ram);
            }
            munmap(reinterpret_cast<void*>(candidate), ram + extra);
            continue;
        }
        bool ok = mmap(reinterpret_cast<void*>(candidate), ram, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) !=
                  MAP_FAILED;
        ok = ok && (extra == 0 || mmap(reinterpret_cast<void*>(candidate + ram), extra, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != MAP_FAILED);
        for (size_t i = 0; ok && i < alias_count; ++i)
        {
            ok = mmap(reinterpret_cast<void*>(candidate + aliases[i]), ram, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_FIXED, fd, 0) != MAP_FAILED;
        }
        if (ok)
        {
            close(fd);  // the mappings keep the memory object alive
            *base = reinterpret_cast<void*>(candidate);
            return true;
        }
        for (size_t i = 0; i < alias_count; ++i)
        {
            munmap(reinterpret_cast<void*>(candidate + aliases[i]), ram);
        }
        munmap(reinterpret_cast<void*>(candidate), ram + extra);
        break;
    }
    close(fd);
    return false;
}

void FreeGuestSpace(void* base, size_t ram, size_t extra, const uintptr_t* aliases, size_t alias_count)
{
    if (base == nullptr)
    {
        return;
    }
    const auto address = reinterpret_cast<uintptr_t>(base);
    for (size_t i = 0; i < alias_count; ++i)
    {
        munmap(reinterpret_cast<void*>(address + aliases[i]), ram);
    }
    munmap(base, ram + extra);
}

}  // namespace rexbox::platform
