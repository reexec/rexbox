// Platform contract: the host call stacks of chosen threads, printed once for
// a diagnostic report (a stalled stream, a stall).
//
// A thread registers itself; PrintThreadStacks then makes each registered
// thread print its own stack to stderr as code addresses by module, which
// addr2line or the linker map turn into function names.

#ifndef REXBOX_PLATFORM_STACK_REPORT_H
#define REXBOX_PLATFORM_STACK_REPORT_H

namespace rexbox::platform
{

// The calling thread takes part in stack reports under `name` (copied).
void RegisterStackReportThread(const char* name);

// Prints each registered thread's host call stack to stderr. False when the
// platform cannot (nothing is printed then). Not for hot paths: it waits for
// each thread in turn.
bool PrintThreadStacks();

}  // namespace rexbox::platform

#endif  // REXBOX_PLATFORM_STACK_REPORT_H
