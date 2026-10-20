// Virtual memory contract on Linux: anonymous mmap, mprotect, and written-page
// queries through the kernel's soft-dirty page bits (CONFIG_MEM_SOFT_DIRTY:
// /proc/self/pagemap bit 55, cleared for the whole process by writing "4" to
// /proc/self/clear_refs). Where the kernel has no soft-dirty tracking (WSL2's
// among them) a tracked block is allocated untracked and TakeWrittenPages
// reports failure, so callers treat every page as written.

#include "rexbox/platform/virtual_memory.h"

#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace rexbox::platform
{

namespace
{

constexpr uint64_t kPagemapSoftDirty = 1ull << 55;

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

// Clears the soft-dirty bits of every mapping of the process.
bool ClearSoftDirty()
{
    FILE* refs = std::fopen("/proc/self/clear_refs", "w");
    if (refs == nullptr)
    {
        return false;
    }
    const bool ok = std::fputs("4\n", refs) >= 0;
    return std::fclose(refs) == 0 && ok;
}

// Reads the pagemap entries of [address, address + size).
bool ReadPagemap(const void* address, size_t size, std::vector<uint64_t>* entries)
{
    FILE* pagemap = std::fopen("/proc/self/pagemap", "rb");
    if (pagemap == nullptr)
    {
        return false;
    }
    const size_t pages = size / PageSize();
    entries->resize(pages);
    const auto offset = static_cast<long>(reinterpret_cast<uintptr_t>(address) / PageSize() * sizeof(uint64_t));
    const bool ok = std::fseek(pagemap, offset, SEEK_SET) == 0 &&
                    std::fread(entries->data(), sizeof(uint64_t), pages, pagemap) == pages;
    std::fclose(pagemap);
    return ok;
}

// Whether this kernel tracks soft-dirty bits: decided once by clearing them,
// writing a page and reading its bit back.
bool SoftDirtySupported()
{
    static const bool supported = [] {
        void* page = mmap(nullptr, PageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED)
        {
            return false;
        }
        bool result = false;
        if (ClearSoftDirty())
        {
            *static_cast<volatile char*>(page) = 1;
            std::vector<uint64_t> entries;
            result = ReadPagemap(page, PageSize(), &entries) && (entries[0] & kPagemapSoftDirty) != 0;
        }
        munmap(page, PageSize());
        return result;
    }();
    return supported;
}

// Blocks allocated with write tracking that the kernel can track.
std::mutex g_tracked_mutex;
std::vector<std::pair<uintptr_t, size_t>> g_tracked;

bool IsTracked(const void* address, size_t size)
{
    const auto begin = reinterpret_cast<uintptr_t>(address);
    std::lock_guard<std::mutex> lock(g_tracked_mutex);
    for (const auto& block : g_tracked)
    {
        if (begin >= block.first && begin + size <= block.first + block.second)
        {
            return true;
        }
    }
    return false;
}

}  // namespace

void* AllocateVirtualMemory(size_t size, void* preferred_address, bool require_preferred, bool track_writes)
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
    if (track_writes && SoftDirtySupported())
    {
        std::lock_guard<std::mutex> lock(g_tracked_mutex);
        g_tracked.emplace_back(reinterpret_cast<uintptr_t>(block), size);
        // The block starts clean: nothing was written since the allocation.
        ClearSoftDirty();
    }
    return block;
}

void FreeVirtualMemory(void* address, size_t size)
{
    if (address == nullptr)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_tracked_mutex);
        for (auto it = g_tracked.begin(); it != g_tracked.end(); ++it)
        {
            if (it->first == reinterpret_cast<uintptr_t>(address))
            {
                g_tracked.erase(it);
                break;
            }
        }
    }
    munmap(address, size);
}

bool TakeWrittenPages(void* address, size_t size, void** pages, size_t capacity, size_t* count)
{
    if (!IsTracked(address, size))
    {
        return false;
    }
    std::vector<uint64_t> entries;
    if (!ReadPagemap(address, size, &entries))
    {
        return false;
    }
    size_t found = 0;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if ((entries[i] & kPagemapSoftDirty) == 0)
        {
            continue;
        }
        if (found == capacity)
        {
            // More written pages than the caller can take: leave the record
            // as it is and report failure, as Win32's GetWriteWatch does.
            return false;
        }
        pages[found++] = static_cast<char*>(address) + i * PageSize();
    }
    // clear_refs clears every mapping of the process, which is more than the
    // range asked for; the only tracked block is the guest memory, so the
    // record of other ranges of it is lost only when they are queried in
    // pieces. Callers query whole buffers, so this is accepted.
    if (!ClearSoftDirty())
    {
        return false;
    }
    *count = found;
    return true;
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
