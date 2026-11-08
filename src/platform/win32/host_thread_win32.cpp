#include "rexbox/platform/host_thread.h"

#include <chrono>
#include <windows.h>

namespace rexbox::platform
{

namespace
{

struct ThreadStart
{
    void (*fn)(void*);
    void* context;
};

DWORD WINAPI ThreadProc(LPVOID parameter)
{
    ThreadStart start = *static_cast<ThreadStart*>(parameter);
    delete static_cast<ThreadStart*>(parameter);
    start.fn(start.context);
    return 0;
}

}  // namespace

HostThread CreateHostThread(void (*fn)(void*), void* context, size_t stack_size)
{
    auto* start = new ThreadStart{fn, context};
    HANDLE handle = CreateThread(nullptr, stack_size, &ThreadProc, start, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (handle == nullptr)
    {
        delete start;
        return 0;
    }
    return reinterpret_cast<HostThread>(handle);
}

void JoinHostThread(HostThread thread)
{
    if (thread != 0)
    {
        HANDLE handle = reinterpret_cast<HANDLE>(thread);
        WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);
    }
}

void SleepMilliseconds(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

uint64_t ThreadCpuTimeNanoseconds()
{
    FILETIME creation = {};
    FILETIME exit = {};
    FILETIME kernel = {};
    FILETIME user = {};
    if (GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user) == 0)
    {
        return 0;
    }
    const auto ticks = [](const FILETIME& t) {
        return (static_cast<uint64_t>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
    };
    return (ticks(kernel) + ticks(user)) * 100;  // FILETIME counts 100 ns
}

namespace
{

double ThreadCyclesToNanoseconds()
{
    static const double ratio = [] {
        const HANDLE thread = GetCurrentThread();
        ULONG64 start_cycles = 0;
        if (!QueryThreadCycleTime(thread, &start_cycles) || start_cycles == 0)
        {
            return 0.0;
        }
        const auto start_time = std::chrono::steady_clock::now();
        const auto target = start_time + std::chrono::milliseconds(10);
        while (std::chrono::steady_clock::now() < target)
        {
        }
        const auto end_time = std::chrono::steady_clock::now();
        ULONG64 end_cycles = 0;
        if (!QueryThreadCycleTime(thread, &end_cycles) || end_cycles <= start_cycles)
        {
            return 0.0;
        }
        const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        return static_cast<double>(elapsed_ns) / static_cast<double>(end_cycles - start_cycles);
    }();
    return ratio;
}

}  // namespace

uint64_t ThreadCpuPreciseNanoseconds()
{
    const double ratio = ThreadCyclesToNanoseconds();
    if (ratio <= 0.0)
    {
        return ThreadCpuTimeNanoseconds();
    }
    ULONG64 cycles = 0;
    if (!QueryThreadCycleTime(GetCurrentThread(), &cycles))
    {
        return ThreadCpuTimeNanoseconds();
    }
    return static_cast<uint64_t>(static_cast<double>(cycles) * ratio);
}

}  // namespace rexbox::platform
