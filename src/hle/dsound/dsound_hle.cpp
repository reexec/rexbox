#include "rexbox/hle/dsound/dsound_hle.h"

#include "dsound_internal.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace rexbox::hle::dsound
{

namespace
{

const std::chrono::steady_clock::time_point g_apu_start = std::chrono::steady_clock::now();

}  // namespace

DsoundState& State()
{
    static DsoundState state;
    return state;
}

runtime::GuestMemory& Memory()
{
    return *runtime::Env().memory;
}

runtime::GuestHeap& Heap()
{
    return *runtime::Env().heap;
}

GuestAddress AllocateGuest(uint32_t size)
{
    const uint32_t bytes = size == 0 ? 1 : size;
    const auto block = bytes <= runtime::GuestHeap::kSmallMax
                           ? Heap().AllocateSmall(bytes)
                           : Heap().Allocate(bytes, runtime::GuestHeap::kPageSize, false);
    if (!block)
    {
        return 0;
    }
    Memory().Fill(*block, 0, size);
    return *block;
}

Buffer* FindBuffer(GuestAddress pointer)
{
    auto& buffers = State().buffers;
    const auto it = buffers.find(pointer);
    return it == buffers.end() ? nullptr : &it->second;
}

void RegisterDsoundHle(runtime::HleRegistry& registry)
{
    DsoundState& state = State();
    // REXBOX_AUDIO=none runs without a host audio device (diagnostics, CI);
    // cursors still advance in real time.
    const char* audio = std::getenv("REXBOX_AUDIO");
    if (audio != nullptr && std::strcmp(audio, "none") == 0 && state.output == DsoundOutput::kHost)
    {
        state.output = DsoundOutput::kNull;
    }
    const char* trace = std::getenv("REXBOX_DSOUND_TRACE");
    state.trace = trace != nullptr && trace[0] != '\0' && std::strcmp(trace, "0") != 0;
    RegisterDeviceExports(registry);
    RegisterBufferExports(registry);
    RegisterEffectsExports(registry);
    Register3dExports(registry);
    RegisterStreamExports(registry);
}

void SetDsoundOutput(DsoundOutput output)
{
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.output = output;
}

void RenderDsoundFrames(float* samples, uint32_t frames)
{
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (samples != nullptr)
    {
        std::memset(samples, 0, sizeof(float) * 2 * frames);
        MixLocked(samples, frames);
        return;
    }
    std::vector<float> scratch(static_cast<size_t>(frames) * 2, 0.0f);
    MixLocked(scratch.data(), frames);
}

}  // namespace rexbox::hle::dsound

extern "C" uint32_t rexbox_dsound_apu_sample_count(void)
{
    const auto elapsed = std::chrono::steady_clock::now() - rexbox::hle::dsound::g_apu_start;
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    // 48 samples per millisecond: us x 48 / 1000 (overflows only after millennia).
    return static_cast<uint32_t>(static_cast<uint64_t>(us) * (rexbox::hle::dsound::kOutputRate / 1000) / 1000);
}
