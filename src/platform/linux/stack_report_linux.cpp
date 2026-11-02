// Stack reports on Linux: each registered thread is sent SIGUSR2 in turn, and
// the handler writes `backtrace` of that thread to stderr. backtrace() is
// called once at registration so that its first call, which loads libgcc,
// never happens inside the handler.

#include "rexbox/platform/stack_report.h"

#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rexbox::platform
{

namespace
{

constexpr int kMaxFrames = 48;
constexpr auto kPerThreadWait = std::chrono::milliseconds(20);

std::mutex g_mutex;
std::vector<std::pair<pthread_t, std::string>> g_threads;
thread_local char t_name[64];

void StackHandler(int)
{
    void* frames[kMaxFrames];
    const int count = backtrace(frames, kMaxFrames);
    char line[128];
    const int length = std::snprintf(line, sizeof(line), "[stack] %s, %d frame(s):\n", t_name, count);
    if (length > 0)
    {
        (void)!write(STDERR_FILENO, line, static_cast<size_t>(length));
    }
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
}

}  // namespace

void RegisterStackReportThread(const char* name)
{
    std::snprintf(t_name, sizeof(t_name), "%s", name != nullptr ? name : "?");
    static std::once_flag once;
    std::call_once(once, [] {
        void* warm[2];
        backtrace(warm, 2);
        struct sigaction action = {};
        action.sa_handler = &StackHandler;
        action.sa_flags = SA_RESTART;
        sigemptyset(&action.sa_mask);
        sigaction(SIGUSR2, &action, nullptr);
    });
    std::lock_guard<std::mutex> lock(g_mutex);
    g_threads.emplace_back(pthread_self(), t_name);
}

bool PrintThreadStacks()
{
    std::vector<std::pair<pthread_t, std::string>> threads;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        threads = g_threads;
    }
    std::fflush(stderr);
    for (const auto& [thread, name] : threads)
    {
        // A thread that has ended is skipped (ESRCH).
        if (pthread_kill(thread, SIGUSR2) == 0)
        {
            std::this_thread::sleep_for(kPerThreadWait);
        }
    }
    return true;
}

}  // namespace rexbox::platform
