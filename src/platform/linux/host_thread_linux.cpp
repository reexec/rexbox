// Host threads on Linux: pthreads with an explicit stack size, and the
// thread's CPU time from CLOCK_THREAD_CPUTIME_ID.

#include "rexbox/platform/host_thread.h"

#include <pthread.h>
#include <time.h>

namespace rexbox::platform
{

namespace
{

struct ThreadStart
{
    void (*fn)(void*);
    void* context;
};

void* ThreadMain(void* parameter)
{
    ThreadStart start = *static_cast<ThreadStart*>(parameter);
    delete static_cast<ThreadStart*>(parameter);
    start.fn(start.context);
    return nullptr;
}

}  // namespace

HostThread CreateHostThread(void (*fn)(void*), void* context, size_t stack_size)
{
    auto* start = new ThreadStart{fn, context};
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    if (stack_size != 0)
    {
        pthread_attr_setstacksize(&attributes, stack_size);
    }
    pthread_t thread;
    const int error = pthread_create(&thread, &attributes, &ThreadMain, start);
    pthread_attr_destroy(&attributes);
    if (error != 0)
    {
        delete start;
        return 0;
    }
    static_assert(sizeof(pthread_t) <= sizeof(HostThread), "pthread_t must fit a HostThread");
    return static_cast<HostThread>(thread);
}

void JoinHostThread(HostThread thread)
{
    if (thread != 0)
    {
        pthread_join(static_cast<pthread_t>(thread), nullptr);
    }
}

void SleepMilliseconds(uint32_t milliseconds)
{
    timespec interval;
    interval.tv_sec = milliseconds / 1000;
    interval.tv_nsec = static_cast<long>(milliseconds % 1000) * 1000000L;
    while (nanosleep(&interval, &interval) != 0)
    {
        // Interrupted by a signal: sleep the remainder.
    }
}

uint64_t ThreadCpuTimeNanoseconds()
{
    timespec now;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0)
    {
        return 0;
    }
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ull + static_cast<uint64_t>(now.tv_nsec);
}

}  // namespace rexbox::platform
