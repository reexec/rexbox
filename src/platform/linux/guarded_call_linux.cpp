// Guarded calls on Linux. A stop request is a C++ exception thrown by
// RaiseStop and caught in RunGuarded, so destructors of the frames between
// them run (what /EHa gives the Win32 implementation). A hardware fault
// arrives as a signal (SIGSEGV, SIGBUS, SIGILL, SIGFPE); the handler records
// it in the innermost RunGuarded's result and leaves through siglongjmp,
// which runs no destructors: a faulted guest thread ends, so nothing of its
// frames is used again.

#include "rexbox/platform/guarded_call.h"

#include <csetjmp>
#include <csignal>
#include <mutex>
#include <ucontext.h>

namespace rexbox::platform
{

namespace
{

struct StopRequest
{
    uint32_t code;
};

// The innermost guarded frame of this thread.
struct Frame
{
    sigjmp_buf jump;
    GuardedCallResult* result;
    Frame* previous;
};

thread_local Frame* t_frame = nullptr;

constexpr int kSignals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE};

void OnFault(int signal, siginfo_t* info, void* raw_context)
{
    Frame* frame = t_frame;
    if (frame == nullptr)
    {
        // Not below a RunGuarded: let the default action end the process.
        std::signal(signal, SIG_DFL);
        raise(signal);
        return;
    }
    GuardedCallResult& result = *frame->result;
    result.outcome = GuardedOutcome::kFault;
    result.fault_code = static_cast<uint32_t>(signal);
    const auto* context = static_cast<const ucontext_t*>(raw_context);
#if defined(__x86_64__)
    result.exception_address = static_cast<uintptr_t>(context->uc_mcontext.gregs[REG_RIP]);
    const bool write = (context->uc_mcontext.gregs[REG_ERR] & 2) != 0;
#elif defined(__i386__)
    result.exception_address = static_cast<uintptr_t>(context->uc_mcontext.gregs[REG_EIP]);
    const bool write = (context->uc_mcontext.gregs[REG_ERR] & 2) != 0;
#else
    (void)context;
    const bool write = false;
#endif
    if (signal == SIGSEGV || signal == SIGBUS)
    {
        result.access_address = reinterpret_cast<uintptr_t>(info->si_addr);
        result.access_is_write = write;
    }
    siglongjmp(frame->jump, 1);
}

void InstallHandlers()
{
    static std::once_flag once;
    std::call_once(once, [] {
        struct sigaction action = {};
        action.sa_sigaction = &OnFault;
        sigemptyset(&action.sa_mask);
        // SA_NODEFER: the handler leaves by siglongjmp, so the signal must not
        // stay blocked for the next fault on this thread.
        action.sa_flags = SA_SIGINFO | SA_NODEFER;
        for (int signal : kSignals)
        {
            sigaction(signal, &action, nullptr);
        }
    });
}

}  // namespace

GuardedCallResult RunGuarded(void (*fn)(void*), void* context)
{
    InstallHandlers();
    GuardedCallResult result;
    Frame frame;
    frame.result = &result;
    frame.previous = t_frame;
    t_frame = &frame;
    if (sigsetjmp(frame.jump, 1) == 0)
    {
        try
        {
            fn(context);
            result.outcome = GuardedOutcome::kReturned;
        }
        catch (const StopRequest& stop)
        {
            result.outcome = GuardedOutcome::kStopRequested;
            result.stop_code = stop.code;
        }
    }
    // Else: the fault handler filled `result` and jumped here.
    t_frame = frame.previous;
    return result;
}

void RaiseStop(uint32_t code)
{
    throw StopRequest{code};
}

}  // namespace rexbox::platform
