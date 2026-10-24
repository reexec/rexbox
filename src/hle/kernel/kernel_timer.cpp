// Kernel timers that fire, and DPC delivery
// (docs/design/20261020-i012-kernel-timers-dpc.md).
//
// A host timer-service thread, which never runs guest code, keeps the armed
// timers and signals each when its deadline passes, under the dispatcher lock
// like every other signal. DPCs -- queued by KeInsertQueueDpc or by a timer --
// run in order on one kernel-owned guest thread at IRQL 2, started at the
// first DPC. Lock order: timer mutex, then the dispatcher lock; the DPC mutex
// is never held while either is taken.

#include "kernel_internal.h"

#include "rexbox/runtime/kernel_thunks.h"
#include "rexbox/runtime/thread_activity.h"

#include <algorithm>
#include <deque>
#include <thread>
#include <unordered_map>

namespace rexbox::hle::kernel
{

namespace
{

using runtime::HleCall;
using Clock = std::chrono::steady_clock;
namespace L = layout;

// KTIMER: header (0x10), DueTime (+0x10, 8 bytes), TimerListEntry (+0x18),
// Dpc (+0x20), Period (+0x24, ms).
constexpr uint32_t kTimerDueTime = 0x10;
constexpr uint32_t kTimerDpc = 0x20;
constexpr uint32_t kTimerPeriod = 0x24;

// KDPC: Type (+0), Inserted (+2), DpcListEntry (+4), DeferredRoutine (+0xC),
// DeferredContext (+0x10), SystemArgument1/2 (+0x14/+0x18).
constexpr uint32_t kDpcInserted = 0x02;
constexpr uint32_t kDpcRoutine = 0x0C;
constexpr uint32_t kDpcContext = 0x10;
constexpr uint32_t kDpcArgument1 = 0x14;
constexpr uint32_t kDpcArgument2 = 0x18;

constexpr uint8_t kDispatchLevel = 2;
constexpr uint32_t kDpcThreadStack = 64 * 1024;

// --- DPC queue -------------------------------------------------------------

struct DpcQueue
{
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<GuestAddress> entries;
    bool thread_started = false;
};

DpcQueue& Dpcs()
{
    static DpcQueue queue;
    return queue;
}

// Starts the DPC thread the first time a DPC is queued.
void EnsureDpcThread()
{
    DpcQueue& queue = Dpcs();
    {
        std::lock_guard<std::mutex> lock(queue.mutex);
        if (queue.thread_started)
        {
            return;
        }
        queue.thread_started = true;
    }
    std::string error;
    const std::shared_ptr<KernelThread> thread =
        StartKernelGuestThread(runtime::KernelThunkVa(kDpcLoopOrdinal), kDpcThreadStack, &error);
    if (thread == nullptr)
    {
        TraceOnce("DPC thread", "not started (" + error + "); DPCs stay queued");
    }
    else
    {
        TraceOnce("DPC thread", "started as thread " + std::to_string(thread->id));
    }
}

// Queues `dpc` with its system arguments unless it is already queued.
bool InsertDpc(GuestAddress dpc, uint32_t argument1, uint32_t argument2)
{
    runtime::GuestMemory& memory = Memory();
    DpcQueue& queue = Dpcs();
    {
        std::lock_guard<std::mutex> lock(queue.mutex);
        if (memory.Read8(dpc + kDpcInserted) != 0)
        {
            return false;
        }
        memory.Write8(dpc + kDpcInserted, 1);
        memory.Write32(dpc + kDpcArgument1, argument1);
        memory.Write32(dpc + kDpcArgument2, argument2);
        queue.entries.push_back(dpc);
    }
    queue.ready.notify_one();
    EnsureDpcThread();
    return true;
}

bool RemoveDpc(GuestAddress dpc)
{
    DpcQueue& queue = Dpcs();
    std::lock_guard<std::mutex> lock(queue.mutex);
    const auto found = std::find(queue.entries.begin(), queue.entries.end(), dpc);
    if (found == queue.entries.end())
    {
        return false;
    }
    queue.entries.erase(found);
    Memory().Write8(dpc + kDpcInserted, 0);
    return true;
}

// The DPC thread's body ("rexbox_DpcLoop", bound at kDpcLoopOrdinal's
// synthetic address): runs queued DPCs forever at IRQL 2.
void DpcLoop()
{
    runtime::GuestMemory& memory = Memory();
    KernelThread* self = CurrentThread();
    runtime::GuestExecutor& executor = *runtime::Env().executor;
    DpcQueue& queue = Dpcs();
    for (;;)
    {
        GuestAddress dpc = 0;
        {
            std::unique_lock<std::mutex> lock(queue.mutex);
            const runtime::ScopedWait note("DPC queue (idle)", nullptr, 0, false, -1);
            queue.ready.wait(lock, [&queue] { return !queue.entries.empty(); });
            dpc = queue.entries.front();
            queue.entries.pop_front();
            memory.Write8(dpc + kDpcInserted, 0);
        }
        const GuestAddress routine = memory.Read32(dpc + kDpcRoutine);
        char routine_text[24];  // "routine " + 8 hex digits + NUL is 17
        std::snprintf(routine_text, sizeof(routine_text), "routine %08X", routine);
        TraceOnce("DPC", routine_text);  // once per routine: which DPCs a title runs
        const uint32_t arguments[4] = {dpc, memory.Read32(dpc + kDpcContext), memory.Read32(dpc + kDpcArgument1),
                                       memory.Read32(dpc + kDpcArgument2)};
        if (self != nullptr)
        {
            memory.Write8(self->kpcr + L::kPcrIrql, kDispatchLevel);
        }
        bool called = false;
        executor.CallGuest(routine, arguments, 4, &called);
        if (self != nullptr)
        {
            memory.Write8(self->kpcr + L::kPcrIrql, 0);
        }
        if (!called)
        {
            TraceOnce("DPC", "routine has no code");
        }
    }
}

// --- Timers ----------------------------------------------------------------

struct ArmedTimer
{
    Clock::time_point deadline;
    uint32_t period_ms = 0;
    GuestAddress dpc = 0;
};

struct TimerService
{
    std::mutex mutex;
    std::condition_variable changed;
    std::unordered_map<GuestAddress, ArmedTimer> armed;
    bool thread_started = false;
};

TimerService& Timers()
{
    static TimerService service;
    return service;
}

// Signals `timer` and queues its DPC; the timer mutex is held.
void FireLocked(GuestAddress timer, const ArmedTimer& armed)
{
    {
        std::lock_guard<std::mutex> lock(State().dispatcher);
        Memory().Write32(timer + L::kHeaderSignalState, 1);
        State().signalled.notify_all();
    }
    if (armed.dpc != 0)
    {
        // SystemArgument1/2: the firing time (NT passes the interrupt time).
        const uint64_t now = InterruptTime100ns();
        InsertDpc(armed.dpc, static_cast<uint32_t>(now), static_cast<uint32_t>(now >> 32));
    }
}

void TimerServiceMain()
{
    TimerService& service = Timers();
    std::unique_lock<std::mutex> lock(service.mutex);
    for (;;)
    {
        const Clock::time_point now = Clock::now();
        std::optional<Clock::time_point> earliest;
        for (auto it = service.armed.begin(); it != service.armed.end();)
        {
            if (it->second.deadline <= now)
            {
                const GuestAddress timer = it->first;
                ArmedTimer armed = it->second;
                if (armed.period_ms != 0)
                {
                    it->second.deadline += std::chrono::milliseconds(armed.period_ms);
                    if (it->second.deadline <= now)
                    {
                        it->second.deadline = now + std::chrono::milliseconds(armed.period_ms);
                    }
                    earliest = earliest ? std::min(*earliest, it->second.deadline) : it->second.deadline;
                    ++it;
                }
                else
                {
                    it = service.armed.erase(it);
                }
                FireLocked(timer, armed);
                continue;
            }
            earliest = earliest ? std::min(*earliest, it->second.deadline) : it->second.deadline;
            ++it;
        }
        if (earliest)
        {
            service.changed.wait_until(lock, *earliest);
        }
        else
        {
            service.changed.wait(lock);
        }
    }
}

// A due time (100 ns units; negative relative, positive absolute system time,
// zero now) as a deadline.
Clock::time_point DeadlineFromDueTime(int64_t due)
{
    const Clock::time_point now = Clock::now();
    if (due <= 0)
    {
        return now + std::chrono::nanoseconds(-due * 100);
    }
    const int64_t until = due - static_cast<int64_t>(SystemTimeNt());
    return now + std::chrono::nanoseconds(until > 0 ? until * 100 : 0);
}

// Arms `timer`; returns whether it was armed already.
bool SetTimer(GuestAddress timer, int64_t due, uint32_t period_ms, GuestAddress dpc)
{
    runtime::GuestMemory& memory = Memory();
    {
        std::lock_guard<std::mutex> lock(State().dispatcher);
        memory.Write32(timer + L::kHeaderSignalState, 0);
    }
    const Clock::time_point deadline = DeadlineFromDueTime(due);
    const uint64_t due_interrupt =
        InterruptTime100ns() +
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - Clock::now()).count() /
                              100);
    memory.Write32(timer + kTimerDueTime, static_cast<uint32_t>(due_interrupt));
    memory.Write32(timer + kTimerDueTime + 4, static_cast<uint32_t>(due_interrupt >> 32));
    memory.Write32(timer + kTimerDpc, dpc);
    memory.Write32(timer + kTimerPeriod, period_ms);

    TimerService& service = Timers();
    bool was_armed = false;
    bool start = false;
    {
        std::lock_guard<std::mutex> lock(service.mutex);
        was_armed = service.armed.count(timer) != 0;
        service.armed[timer] = ArmedTimer{deadline, period_ms, dpc};
        start = !service.thread_started;
        service.thread_started = true;
    }
    if (start)
    {
        // Detached like the tick thread: it sleeps on a condition variable
        // and ends with the process.
        std::thread(&TimerServiceMain).detach();
    }
    service.changed.notify_one();
    return was_armed;
}

int64_t DueTimeArgument(uint32_t low, uint32_t high)
{
    return static_cast<int64_t>(static_cast<uint64_t>(low) | (static_cast<uint64_t>(high) << 32));
}

// BOOLEAN KeSetTimer(PKTIMER Timer, LARGE_INTEGER DueTime, PKDPC Dpc)
void KeSetTimer()
{
    HleCall call;
    const bool was_armed = SetTimer(call.Arg(0), DueTimeArgument(call.Arg(1), call.Arg(2)), 0, call.Arg(3));
    call.Return(was_armed ? 1 : 0, 4);
}

// BOOLEAN KeSetTimerEx(PKTIMER Timer, LARGE_INTEGER DueTime, LONG Period, PKDPC Dpc)
void KeSetTimerEx()
{
    HleCall call;
    const int32_t period = static_cast<int32_t>(call.Arg(3));
    const bool was_armed = SetTimer(call.Arg(0), DueTimeArgument(call.Arg(1), call.Arg(2)),
                                    period > 0 ? static_cast<uint32_t>(period) : 0, call.Arg(4));
    call.Return(was_armed ? 1 : 0, 5);
}

// BOOLEAN KeCancelTimer(PKTIMER Timer)
void KeCancelTimer()
{
    HleCall call;
    TimerService& service = Timers();
    bool was_armed = false;
    {
        std::lock_guard<std::mutex> lock(service.mutex);
        was_armed = service.armed.erase(call.Arg(0)) != 0;
    }
    call.Return(was_armed ? 1 : 0, 1);
}

// BOOLEAN KeInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2)
void KeInsertQueueDpc()
{
    HleCall call;
    call.Return(InsertDpc(call.Arg(0), call.Arg(1), call.Arg(2)) ? 1 : 0, 3);
}

// BOOLEAN KeRemoveQueueDpc(PRKDPC Dpc)
void KeRemoveQueueDpc()
{
    HleCall call;
    call.Return(RemoveDpc(call.Arg(0)) ? 1 : 0, 1);
}

}  // namespace

void RegisterTimerExports(runtime::HleRegistry& registry)
{
    registry.AddExport("kernel", "KeSetTimer", &KeSetTimer);
    registry.AddExport("kernel", "KeSetTimerEx", &KeSetTimerEx);
    registry.AddExport("kernel", "KeCancelTimer", &KeCancelTimer);
    registry.AddExport("kernel", "KeInsertQueueDpc", &KeInsertQueueDpc);
    registry.AddExport("kernel", "KeRemoveQueueDpc", &KeRemoveQueueDpc);
    registry.AddExport("kernel", "rexbox_DpcLoop", &DpcLoop);
}

}  // namespace rexbox::hle::kernel
