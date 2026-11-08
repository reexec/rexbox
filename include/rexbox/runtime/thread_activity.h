// What each guest thread is doing, for a report when presents stop
// (docs/design/20261015-i060-stall-report.md) or another module asks.
//
// HLE entry points note the call a thread last made, the kernel notes its
// waits, and the display notes presents. A monitor thread watches the presents;
// when none comes for REXBOX_STALL_REPORT seconds (default 10, 0 turns it
// off) it prints one line per guest thread to stderr, once per stall, with
// return addresses guessed from the guest stack at its last HLE call; with
// REXBOX_STALL_DUMP=<file> it also writes the guest address space there.
//
// Every note is a few relaxed stores into the calling thread's own record, so
// they are cheap enough to stay on in normal runs.

#ifndef REXBOX_RUNTIME_THREAD_ACTIVITY_H
#define REXBOX_RUNTIME_THREAD_ACTIVITY_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace rexbox::runtime
{

using GuestAddress = uint32_t;

// Names the calling guest thread in reports, e.g. "thread 3 (00147EBB)";
// `guest_id` is passed to the thread describer.
void NoteThreadName(std::string name, uint32_t guest_id);

// The calling thread's name as given to NoteThreadName, or "a host thread".
std::string CurrentThreadName();

// Prints "<prefix> <why>; guest threads:", one line per guest thread as the
// stall report does, and each guest thread's host stack where the platform
// can (for other modules' one-time reports, such as a stalled audio stream).
// Waits for each thread in turn: never from a thread that must not stall.
void ReportGuestThreads(const char* prefix, const char* why);

// The calling thread entered an HLE function. `name` must outlive the process
// (a string literal or a registry entry); `esp` is the guest stack pointer
// at the call, so [esp] is the return address. Runs on every HLE call, so it
// only stores: the time is the monitor's coarse clock (0.1 s steps).
void NoteHleEntry(const char* name, uint32_t esp);

// The calling thread starts or ends a wait in `what` (static storage) on
// `count` objects, all of them or any; `timeout_ms` < 0 means no timeout.
// For its length the thread's guest locks are put down (guest_lock.h).
void NoteWaitBegin(const char* what, const GuestAddress* objects, size_t count, bool all, int64_t timeout_ms);
void NoteWaitEnd();

// NoteWaitBegin for the lifetime of the object.
class ScopedWait
{
public:
    ScopedWait(const char* what, const GuestAddress* objects, size_t count, bool all, int64_t timeout_ms)
    {
        NoteWaitBegin(what, objects, count, all, timeout_ms);
    }
    ~ScopedWait() { NoteWaitEnd(); }
    ScopedWait(const ScopedWait&) = delete;
    ScopedWait& operator=(const ScopedWait&) = delete;
};

// Totals of the calling thread's outermost waits and file reads since the last
// TakeWaitTotals().
struct WaitTotals
{
    uint64_t wait_ns = 0;
    uint32_t wait_count = 0;
    uint64_t longest_wait_ns = 0;
    const char* longest_what = nullptr;
    uint32_t read_count = 0;
    uint64_t read_bytes = 0;
    uint64_t read_ns = 0;
};

// Turns on per-thread wait totals. Once on they stay on.
void EnableWaitTotals();

// Whether wait totals are enabled.
bool WaitTotalsEnabled();

// Records a file read of `bytes` taking `ns` nanoseconds.
void NoteFileRead(uint64_t bytes, uint64_t ns);

// Returns and resets the calling thread's wait totals.
WaitTotals TakeWaitTotals();

// A frame was presented.
void NotePresent(uint32_t frame);

// Describes a wait object for a report (its kind and current signal state).
// The kernel HLE registers one; without it only the address is printed.
using WaitObjectDescriber = std::string (*)(GuestAddress object);
void SetWaitObjectDescriber(WaitObjectDescriber describer);

// Describes a guest thread by the id given to NoteThreadName (the kernel's
// suspension count, priority and handles, say).
using ThreadDescriber = std::string (*)(uint32_t guest_id);
void SetThreadDescriber(ThreadDescriber describer);

// Starts the monitor thread unless REXBOX_STALL_REPORT=0.
void StartStallMonitor();

}  // namespace rexbox::runtime

#endif  // REXBOX_RUNTIME_THREAD_ACTIVITY_H
