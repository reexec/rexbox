#include "rexbox/platform/virtual_memory.h"

#include <windows.h>

namespace rexbox::platform
{

namespace
{

DWORD ToWin32Protection(PageAccess access)
{
    switch (access)
    {
    case PageAccess::kNone:
        return PAGE_NOACCESS;
    case PageAccess::kRead:
        return PAGE_READONLY;
    case PageAccess::kReadWrite:
        return PAGE_READWRITE;
    case PageAccess::kReadExecute:
        return PAGE_EXECUTE_READ;
    case PageAccess::kReadWriteExecute:
        return PAGE_EXECUTE_READWRITE;
    }
    return PAGE_NOACCESS;
}

}  // namespace

void* AllocateVirtualMemory(size_t size, void* preferred_address, bool require_preferred, bool track_writes)
{
    const DWORD type = MEM_RESERVE | MEM_COMMIT | (track_writes ? MEM_WRITE_WATCH : 0);
    if (preferred_address != nullptr)
    {
        void* placed = VirtualAlloc(preferred_address, size, type, PAGE_READWRITE);
        if (placed != nullptr || require_preferred)
        {
            return placed;
        }
    }
    void* block = VirtualAlloc(nullptr, size, type, PAGE_READWRITE);
    if (block == nullptr && track_writes)
    {
        // Write watching is optional; TakeWrittenPages then reports failure.
        block = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }
    return block;
}

void FreeVirtualMemory(void* address, size_t /*size*/)
{
    if (address != nullptr)
    {
        VirtualFree(address, 0, MEM_RELEASE);
    }
}

bool TakeWrittenPages(void* address, size_t size, void** pages, size_t capacity, size_t* count)
{
    ULONG_PTR found = capacity;
    DWORD granularity = 0;
    if (GetWriteWatch(WRITE_WATCH_FLAG_RESET, address, size, pages, &found, &granularity) != 0)
    {
        return false;
    }
    // A full array may have left pages out (and unreset): report failure, so
    // the caller assumes them all written.
    if (found >= capacity && size / PageSize() > capacity)
    {
        return false;
    }
    *count = found;
    return true;
}

bool ProtectVirtualMemory(void* address, size_t size, PageAccess access)
{
    DWORD previous = 0;
    return VirtualProtect(address, size, ToWin32Protection(access), &previous) != FALSE;
}

size_t PageSize()
{
    static const size_t page_size = [] {
        SYSTEM_INFO info = {};
        GetSystemInfo(&info);
        return static_cast<size_t>(info.dwPageSize);
    }();
    return page_size;
}

namespace
{

// The address an alias of `base` lands at: the sum modulo the pointer width.
uintptr_t AliasAddress(uintptr_t base, uintptr_t alias)
{
    return base + alias;
}

// Whether [address, address + size) is entirely free, by walking the regions
// VirtualQuery reports.
bool IsFree(uintptr_t address, size_t size)
{
    uintptr_t at = address;
    while (at < address + size)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (VirtualQuery(reinterpret_cast<void*>(at), &info, sizeof(info)) == 0 || info.State != MEM_FREE)
        {
            return false;
        }
        const uintptr_t region_end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (region_end <= at)
        {
            return false;
        }
        at = region_end;
    }
    return true;
}

// The next free address at or after `from`, 64 KiB aligned, or 0.
uintptr_t NextFree(uintptr_t from)
{
    const uintptr_t granularity = 0x10000;
    uintptr_t at = (from + granularity - 1) & ~(granularity - 1);
    for (;;)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (at == 0 || VirtualQuery(reinterpret_cast<void*>(at), &info, sizeof(info)) == 0)
        {
            return 0;
        }
        if (info.State == MEM_FREE)
        {
            return at;
        }
        const uintptr_t region_end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (region_end <= at)
        {
            return 0;
        }
        at = (region_end + granularity - 1) & ~(granularity - 1);
    }
}

}  // namespace

bool AllocateGuestSpace(size_t ram, size_t extra, const uintptr_t* aliases, size_t alias_count, void** base)
{
    // One pagefile-backed section holds the RAM; views of it are the RAM
    // and its aliases (as xboxrecomp's runtime maps the contiguous window).
    HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(ram), nullptr);
    if (section == nullptr)
    {
        return false;
    }
    // Candidate bases: free 64 KiB aligned addresses, lowest first, whose RAM
    // plus extra range and every alias range are free. The views are mapped
    // right after the check; a lost race shows as a failed map and the next
    // candidate is tried.
    for (uintptr_t candidate = NextFree(0x10000); candidate != 0; candidate = NextFree(candidate + 0x10000))
    {
        bool fits = IsFree(candidate, ram + extra);
        for (size_t i = 0; fits && i < alias_count; ++i)
        {
            fits = IsFree(AliasAddress(candidate, aliases[i]), ram);
        }
        if (!fits)
        {
            continue;
        }
        void* placed = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, ram, reinterpret_cast<void*>(candidate));
        if (placed == nullptr)
        {
            continue;
        }
        bool ok = extra == 0 || VirtualAlloc(reinterpret_cast<void*>(candidate + ram), extra, MEM_RESERVE | MEM_COMMIT,
                                             PAGE_READWRITE) != nullptr;
        size_t mapped = 0;
        for (; ok && mapped < alias_count; ++mapped)
        {
            ok = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, ram,
                                 reinterpret_cast<void*>(AliasAddress(candidate, aliases[mapped]))) != nullptr;
        }
        if (ok)
        {
            // The section handle may close: the views keep it alive.
            CloseHandle(section);
            *base = placed;
            return true;
        }
        for (size_t i = 0; i < mapped; ++i)
        {
            UnmapViewOfFile(reinterpret_cast<void*>(AliasAddress(candidate, aliases[i])));
        }
        if (extra != 0)
        {
            VirtualFree(reinterpret_cast<void*>(candidate + ram), 0, MEM_RELEASE);
        }
        UnmapViewOfFile(placed);
    }
    CloseHandle(section);
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
        UnmapViewOfFile(reinterpret_cast<void*>(AliasAddress(address, aliases[i])));
    }
    if (extra != 0)
    {
        VirtualFree(reinterpret_cast<void*>(address + ram), 0, MEM_RELEASE);
    }
    UnmapViewOfFile(base);
}

}  // namespace rexbox::platform
