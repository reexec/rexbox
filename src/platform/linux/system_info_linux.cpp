// System information contract on Linux: the process's memory from
// /proc/self/statm (pages: size, resident, shared, text, lib, data, dirty).

#include "rexbox/platform/system_info.h"

#include <unistd.h>

#include <cstdio>

namespace rexbox::platform
{

ProcessMemory QueryProcessMemory()
{
    ProcessMemory result;
    FILE* statm = std::fopen("/proc/self/statm", "r");
    if (statm == nullptr)
    {
        return result;
    }
    unsigned long size = 0;
    unsigned long resident = 0;
    unsigned long shared = 0;
    unsigned long text = 0;
    unsigned long lib = 0;
    unsigned long data = 0;
    const int fields = std::fscanf(statm, "%lu %lu %lu %lu %lu %lu", &size, &resident, &shared, &text, &lib, &data);
    std::fclose(statm);
    if (fields < 6)
    {
        return result;
    }
    const auto page = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
    result.known = true;
    result.working_set = resident * page;
    result.private_bytes = data * page;  // data + stack: the process's private writable memory
    result.address_space_used = size * page;
    // The user address space: 3 GiB for a 32-bit process, 128 TiB (47 bits)
    // for a 64-bit one on x86-64.
    result.address_space_total = sizeof(void*) == 4 ? 3ull * 1024 * 1024 * 1024 : 1ull << 47;
    return result;
}

}  // namespace rexbox::platform
