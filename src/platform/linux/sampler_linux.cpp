// Sampling profiler contract on Linux: not implemented yet. The Win32
// sampler suspends the target thread and reads its instruction pointer; the
// Linux equivalent (a timer signal delivered to the target thread, reading
// the program counter from its ucontext) is a follow-up. REXBOX_SAMPLE is
// therefore refused here, and the rest of the runtime does not depend on it.

#include "rexbox/platform/sampler.h"

namespace rexbox::platform
{

bool StartThreadSampling()
{
    return false;
}

uint64_t StopThreadSampling(const char* /*path*/)
{
    return 0;
}

}  // namespace rexbox::platform
