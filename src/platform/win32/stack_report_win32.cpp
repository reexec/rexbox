// Stack reports on Win32: not implemented yet (another thread's stack needs
// SuspendThread and a stack walk); registration is kept for when it is.

#include "rexbox/platform/stack_report.h"

namespace rexbox::platform
{

void RegisterStackReportThread(const char*)
{
}

bool PrintThreadStacks()
{
    return false;
}

}  // namespace rexbox::platform
