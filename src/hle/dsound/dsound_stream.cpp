// DirectSound streams (issue #54, docs/design/20261103-i054-dsound-streams.md):
// an IDirectSoundStream is a voice fed by a queue of XMEDIAPACKETs. The host
// mixer consumes the packets' PCM; packets it finishes are completed on a
// guest thread (DirectSoundDoWork, Process, GetStatus), where the completed
// size and status are written and the stream's callback runs or the packet's
// event is set.
//
// The guest object is a block whose first DWORD points at a guest vtable in
// XDK 5849's order (AddRef, Release, GetInfo, GetStatus, Process,
// Discontinuity, Flush, destructor); its entries are synthetic code
// addresses (0xFF000000 and up) bound to the exports below, so the title's
// indirect calls reach them.

#include "dsound_internal.h"

#include "rexbox/runtime/guest_cpu.h"
#include "rexbox/runtime/kernel_thunks.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rexbox::hle::dsound
{

namespace
{

using runtime::Gpr;

// XMEDIAPACKET.
constexpr uint32_t kPacketBuffer = 0x00;
constexpr uint32_t kPacketMaxSize = 0x04;
constexpr uint32_t kPacketCompletedSize = 0x08;  // DWORD*
constexpr uint32_t kPacketStatus = 0x0C;         // DWORD*
constexpr uint32_t kPacketEvent = 0x10;          // event, or the packet context when the stream has a callback
// DSSTREAMDESC (XDK 5849, the title's 0x0007E4BC).
constexpr uint32_t kDescFlags = 0x00;
constexpr uint32_t kDescMaxPackets = 0x04;
constexpr uint32_t kDescFormat = 0x08;
constexpr uint32_t kDescCallback = 0x0C;
constexpr uint32_t kDescContext = 0x10;
constexpr uint32_t kDescMixBins = 0x14;
// XMEDIAPACKET status values (the HRESULTs the XDK names; **inferred**).
constexpr uint32_t kPacketSuccess = 0;
constexpr uint32_t kPacketPending = 0x8000000A;  // E_PENDING
constexpr uint32_t kPacketFlushed = 0x80004004;  // E_ABORT
// XMO_STATUSF_ACCEPT_INPUT_DATA.
constexpr uint32_t kStatusAcceptInput = 0x1;
// XMO_STREAMF_* in XMEDIAINFO.dwFlags: whole packets, any size.
constexpr uint32_t kInfoFlags = 0x0;
// Trace diagnostics (design i071).
constexpr uint64_t kMaxLoggedCalls = 8;
constexpr std::chrono::seconds kReportInterval{5};

// The vtable's synthetic code addresses (rexbox/runtime/kernel_thunks.h).
constexpr GuestAddress kMethodVaBase = runtime::kHleMethodVaBase;
constexpr const char* kMethodNames[] = {
    "DSStream_AddRef",        "DSStream_Release", "DSStream_GetInfo", "DSStream_GetStatus",
    "DSStream_Process",       "DSStream_Discontinuity", "DSStream_Flush", "DSStream_Destructor",
};

struct Packet
{
    GuestAddress packet = 0;  // the XMEDIAPACKET (its fields are read at Process)
    GuestAddress buffer = 0;
    uint32_t bytes = 0;
    GuestAddress completed_size = 0;
    GuestAddress status = 0;
    uint32_t event_or_context = 0;
    uint32_t consumed = 0;  // bytes the mixer has played
    bool done = false;      // played out or flushed; waiting to be completed
    uint32_t result = kPacketSuccess;
};

struct Stream
{
    uint32_t refs = 1;
    WaveFormat format;
    bool valid_format = false;
    uint32_t max_packets = 0;
    GuestAddress callback = 0;
    uint32_t context = 0;
    bool paused = false;
    int32_t volume = 0;     // hundredths of a dB
    uint32_t headroom = 0;  // hundredths of a dB
    uint32_t frequency = 0; // SetFrequency/SetPitch; 0: the format's rate
    double position = 0.0;  // fractional frame within the front packet
    std::deque<Packet> packets;

    // Packet statistics (design i071).
    uint64_t submitted = 0;
    uint64_t submitted_bytes = 0;
    uint64_t mixed_out = 0;
    uint64_t completed = 0;
    uint64_t starved_mixes = 0;
    uint64_t paused_mixes = 0;
    uint64_t flushes = 0;
    uint64_t discontinuities = 0;
    uint64_t pause_calls = 0;
    std::chrono::steady_clock::time_point last_delivery{};
    float peak_raw = 0.0f;
    float peak_out = 0.0f;
};

std::unordered_map<GuestAddress, Stream> g_streams;  // guarded by State().mutex
std::chrono::steady_clock::time_point g_last_report{};
GuestAddress g_vtable = 0;

Stream* FindStream(GuestAddress pointer)
{
    const auto it = g_streams.find(pointer);
    return it == g_streams.end() ? nullptr : &it->second;
}

float Gain(const Stream& stream)
{
    return std::pow(10.0f, static_cast<float>(stream.volume - static_cast<int32_t>(stream.headroom)) / 2000.0f);
}

float SampleOf(const Packet& packet, const Stream& stream, uint32_t frame, uint32_t channel)
{
    const uint32_t block = stream.format.block_align;
    const uint32_t offset = frame * block + channel * (stream.format.bits / 8);
    if (offset + stream.format.bits / 8 > packet.bytes)
    {
        return 0.0f;
    }
    const uint8_t* data = Memory().Translate(packet.buffer + offset);
    if (stream.format.bits == 16)
    {
        return static_cast<float>(static_cast<int16_t>(data[0] | (data[1] << 8))) / 32768.0f;
    }
    return (static_cast<float>(data[0]) - 128.0f) / 128.0f;
}

// Plays the front packets into `samples`; the mixer holds the mutex.
void MixStream(Stream& stream, float* samples, uint32_t frames)
{
    if (stream.paused)
    {
        ++stream.paused_mixes;
        return;
    }
    if (!stream.valid_format)
    {
        return;
    }
    const bool trace = State().trace;
    const uint32_t rate = stream.frequency != 0 ? stream.frequency : stream.format.rate;
    const double step = static_cast<double>(rate) / static_cast<double>(kOutputRate);
    const uint32_t channels = stream.format.channels;
    const float gain = Gain(stream);
    uint32_t i = 0;
    for (Packet& packet : stream.packets)
    {
        if (packet.done)
        {
            continue;
        }
        const uint32_t packet_frames = packet.bytes / stream.format.block_align;
        while (i < frames)
        {
            const auto frame = static_cast<uint32_t>(stream.position);
            if (frame >= packet_frames)
            {
                break;
            }
            float left = 0.0f;
            float right = 0.0f;
            if (channels == 1)
            {
                left = right = SampleOf(packet, stream, frame, 0);
            }
            else
            {
                left = SampleOf(packet, stream, frame, 0);
                right = SampleOf(packet, stream, frame, 1);
            }
            const float out_left = left * gain;
            const float out_right = right * gain;
            if (trace)
            {
                const float raw_abs = std::max(std::fabs(left), std::fabs(right));
                const float out_abs = std::max(std::fabs(out_left), std::fabs(out_right));
                stream.peak_raw = std::max(stream.peak_raw, raw_abs);
                stream.peak_out = std::max(stream.peak_out, out_abs);
            }
            samples[2 * i] += out_left;
            samples[2 * i + 1] += out_right;
            stream.position += step;
            ++i;
        }
        const auto frame = static_cast<uint32_t>(stream.position);
        if (frame < packet_frames)
        {
            packet.consumed = frame * stream.format.block_align;
            return;  // the output is full
        }
        packet.consumed = packet.bytes;
        packet.done = true;
        ++stream.mixed_out;
        stream.position -= static_cast<double>(packet_frames);
        if (i >= frames)
        {
            return;
        }
    }
    // Starved: no packet left to play (the remaining output stays silent).
    if (i < frames)
    {
        ++stream.starved_mixes;
    }
    stream.position = 0.0;
}

// Sets a guest event through the kernel's export, on this guest thread.
void SetGuestEvent(uint32_t handle)
{
    const runtime::HleExport* set_event = runtime::Env().registry->FindExport("NtSetEvent");
    if (set_event == nullptr || set_event->thunk == nullptr)
    {
        return;
    }
    runtime::GuestCpu& cpu = runtime::Env().executor->Cpu();
    const uint32_t esp = cpu.Get(Gpr::kEsp);
    const uint32_t eax = cpu.Get(Gpr::kEax);
    // NTSTATUS NtSetEvent(HANDLE, PLONG PreviousState): a stdcall frame below
    // the current stack; its Return pops it again.
    const uint32_t frame = esp - 12;
    Memory().Write32(frame, 0);  // return address (unused: the thunk returns to us)
    Memory().Write32(frame + 4, handle);
    Memory().Write32(frame + 8, 0);
    cpu.Set(Gpr::kEsp, frame);
    set_event->thunk();
    cpu.Set(Gpr::kEsp, esp);
    cpu.Set(Gpr::kEax, eax);
}

// Completes the packets the mixer finished, in order, on the calling guest
// thread. Takes and releases the mutex itself: the callback may call back
// into DirectSound.
void DeliverCompletions()
{
    struct Completion
    {
        GuestAddress completed_size;
        GuestAddress status;
        uint32_t bytes;
        uint32_t result;
        uint32_t event_or_context;
        GuestAddress callback;
        uint32_t context;
    };
    std::vector<Completion> completions;
    {
        std::lock_guard<std::mutex> lock(State().mutex);
        const auto now = std::chrono::steady_clock::now();
        for (auto& [pointer, stream] : g_streams)
        {
            while (!stream.packets.empty() && stream.packets.front().done)
            {
                const Packet& packet = stream.packets.front();
                completions.push_back({packet.completed_size, packet.status, packet.consumed, packet.result,
                                       packet.event_or_context, stream.callback, stream.context});
                stream.packets.pop_front();
                ++stream.completed;
                stream.last_delivery = now;
            }
        }
    }
    runtime::GuestMemory& memory = Memory();
    for (const Completion& c : completions)
    {
        if (c.completed_size != 0)
        {
            memory.Write32(c.completed_size, c.bytes);
        }
        if (c.status != 0)
        {
            memory.Write32(c.status, c.result);
        }
        if (c.callback != 0)
        {
            const uint32_t arguments[3] = {c.context, c.event_or_context, c.result};
            bool called = false;
            runtime::Env().executor->CallGuest(c.callback, arguments, 3, &called);
        }
        else if (c.event_or_context != 0)
        {
            SetGuestEvent(c.event_or_context);
        }
    }
}

// Marks every queued packet flushed (Flush, Release to zero).
void FlushLocked(Stream& stream)
{
    for (Packet& packet : stream.packets)
    {
        if (!packet.done)
        {
            packet.done = true;
            packet.result = kPacketFlushed;
        }
    }
    stream.position = 0.0;
}

void InitializeVtable();

GuestAddress CreateStream(GuestAddress desc)
{
    InitializeVtable();
    runtime::GuestMemory& memory = Memory();
    Stream stream;
    stream.max_packets = std::max<uint32_t>(memory.Read32(desc + kDescMaxPackets), 1);
    stream.callback = memory.Read32(desc + kDescCallback);
    stream.context = memory.Read32(desc + kDescContext);
    const GuestAddress format = memory.Read32(desc + kDescFormat);
    stream.valid_format = format != 0 && ReadFormat(format, &stream.format) &&
                          stream.format.tag == kFormatPcm && (stream.format.bits == 8 || stream.format.bits == 16) &&
                          stream.format.block_align != 0;
    if (!stream.valid_format)
    {
        std::fprintf(stderr, "[dsound] stream format %04X, %u bit(s) is not mixed (PCM 8/16 only)\n",
                     stream.format.tag, stream.format.bits);
    }
    const GuestAddress object = AllocateGuest(kObjectBlockSize);
    if (object == 0)
    {
        return 0;
    }
    memory.Write32(object, g_vtable);
    if (State().trace)
    {
        std::fprintf(stderr, "[dsound] stream %08X: %u ch, %u Hz, %u bit, %u packets, callback %08X, flags %08X, mix bins %08X\n",
                     object, stream.format.channels, stream.format.rate, stream.format.bits, stream.max_packets,
                     stream.callback, memory.Read32(desc + kDescFlags), memory.Read32(desc + kDescMixBins));
    }
    std::lock_guard<std::mutex> lock(State().mutex);
    StartOutputLocked();
    g_streams.emplace(object, std::move(stream));
    return object;
}

// HRESULT DirectSoundCreateStream(LPCDSSTREAMDESC pdssd, LPDIRECTSOUNDSTREAM* ppStream)
void DirectSoundCreateStream()
{
    HleCall call;
    const GuestAddress out = call.Arg(1);
    if (call.Arg(0) == 0 || out == 0)
    {
        call.Return(kErrInvalidCall, 2);
        return;
    }
    const GuestAddress object = CreateStream(call.Arg(0));
    Memory().Write32(out, object);
    call.Return(object != 0 ? kDsOk : kErrOutOfMemory, 2);
}

// HRESULT IDirectSound_CreateSoundStream(LPDIRECTSOUND, LPCDSSTREAMDESC, LPDIRECTSOUNDSTREAM*, LPUNKNOWN)
void IDirectSound_CreateSoundStream()
{
    HleCall call;
    const GuestAddress out = call.Arg(2);
    if (call.Arg(1) == 0 || out == 0)
    {
        call.Return(kErrInvalidCall, 4);
        return;
    }
    const GuestAddress object = CreateStream(call.Arg(1));
    Memory().Write32(out, object);
    call.Return(object != 0 ? kDsOk : kErrOutOfMemory, 4);
}

// ULONG AddRef(this)
void DSStream_AddRef()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(State().mutex);
    Stream* stream = FindStream(call.Arg(0));
    call.Return(stream != nullptr ? ++stream->refs : 0, 1);
}

// ULONG Release(this)
void DSStream_Release()
{
    HleCall call;
    uint32_t refs = 0;
    {
        std::lock_guard<std::mutex> lock(State().mutex);
        Stream* stream = FindStream(call.Arg(0));
        if (stream != nullptr && stream->refs > 0)
        {
            refs = --stream->refs;
            if (refs == 0)
            {
                FlushLocked(*stream);
            }
        }
    }
    DeliverCompletions();
    if (refs == 0)
    {
        std::lock_guard<std::mutex> lock(State().mutex);
        g_streams.erase(call.Arg(0));
    }
    call.Return(refs, 1);
}

// HRESULT GetInfo(this, LPXMEDIAINFO): {dwFlags, dwInputSize, dwOutputSize, dwMaxLookahead}
void DSStream_GetInfo()
{
    HleCall call;
    const GuestAddress info = call.Arg(1);
    if (info != 0)
    {
        std::lock_guard<std::mutex> lock(State().mutex);
        const Stream* stream = FindStream(call.Arg(0));
        const uint32_t block = stream != nullptr ? stream->format.block_align : 0;
        Memory().Write32(info + 0x0, kInfoFlags);
        Memory().Write32(info + 0x4, block);
        Memory().Write32(info + 0x8, 0);
        Memory().Write32(info + 0xC, 0);
    }
    call.Return(kDsOk, 2);
}

// HRESULT GetStatus(this, LPDWORD pdwStatus): room for another packet?
void DSStream_GetStatus()
{
    HleCall call;
    DeliverCompletions();
    uint32_t status = 0;
    {
        std::lock_guard<std::mutex> lock(State().mutex);
        const Stream* stream = FindStream(call.Arg(0));
        if (stream != nullptr && stream->packets.size() < stream->max_packets)
        {
            status = kStatusAcceptInput;
        }
    }
    if (call.Arg(1) != 0)
    {
        Memory().Write32(call.Arg(1), status);
    }
    call.Return(kDsOk, 2);
}

// HRESULT Process(this, LPCXMEDIAPACKET pInputPacket, LPCXMEDIAPACKET pOutputPacket)
void DSStream_Process()
{
    HleCall call;
    DeliverCompletions();
    const GuestAddress pointer = call.Arg(0);
    const GuestAddress input = call.Arg(1);
    if (input == 0)
    {
        call.Return(kErrInvalidCall, 3);
        return;
    }
    runtime::GuestMemory& memory = Memory();
    Packet packet;
    packet.packet = input;
    packet.buffer = memory.Read32(input + kPacketBuffer);
    packet.bytes = memory.Read32(input + kPacketMaxSize);
    packet.completed_size = memory.Read32(input + kPacketCompletedSize);
    packet.status = memory.Read32(input + kPacketStatus);
    packet.event_or_context = memory.Read32(input + kPacketEvent);
    if (packet.status != 0)
    {
        memory.Write32(packet.status, kPacketPending);
    }
    if (packet.completed_size != 0)
    {
        memory.Write32(packet.completed_size, 0);
    }
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    Stream* stream = FindStream(pointer);
    if (stream == nullptr)
    {
        call.Return(kErrInvalidCall, 3);
        return;
    }
    if (!stream->valid_format || packet.buffer == 0 || packet.bytes == 0)
    {
        packet.done = true;  // completed at once, nothing played
    }
    stream->packets.push_back(packet);
    ++stream->submitted;
    stream->submitted_bytes += packet.bytes;
    if (state.trace && stream->submitted <= kMaxLoggedCalls)
    {
        std::fprintf(stderr,
                     "[dsound] stream %08X Process #%llu: buffer %08X, %u bytes, event/context %08X (queued %zu)\n",
                     pointer, static_cast<unsigned long long>(stream->submitted), packet.buffer, packet.bytes,
                     packet.event_or_context, stream->packets.size());
    }
    call.Return(kDsOk, 3);
}

// HRESULT Discontinuity(this): the data so far is the end of a run; the HLE
// keeps playing what is queued, which is what that means.
void DSStream_Discontinuity()
{
    HleCall call;
    const GuestAddress pointer = call.Arg(0);
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    Stream* stream = FindStream(pointer);
    if (stream != nullptr)
    {
        ++stream->discontinuities;
        if (state.trace && stream->discontinuities <= kMaxLoggedCalls)
        {
            std::fprintf(stderr, "[dsound] stream %08X Discontinuity #%llu\n", pointer,
                         static_cast<unsigned long long>(stream->discontinuities));
        }
    }
    call.Return(kDsOk, 1);
}

// HRESULT Flush(this)
void DSStream_Flush()
{
    HleCall call;
    const GuestAddress pointer = call.Arg(0);
    {
        DsoundState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        Stream* stream = FindStream(pointer);
        if (stream != nullptr)
        {
            ++stream->flushes;
            if (state.trace && stream->flushes <= kMaxLoggedCalls)
            {
                std::fprintf(stderr, "[dsound] stream %08X Flush #%llu\n", pointer,
                             static_cast<unsigned long long>(stream->flushes));
            }
            FlushLocked(*stream);
        }
    }
    DeliverCompletions();
    call.Return(kDsOk, 1);
}

// The scalar deleting destructor (this, flags), thiscall in the library; the
// title does not call it through the vtable (Release does). Pops nothing.
void DSStream_Destructor()
{
    HleCall call;
    call.Return(call.Ecx(), 1);
}

// HRESULT IDirectSoundStream_Pause(LPDIRECTSOUNDSTREAM, DWORD dwPause): 0
// resumes, 1 pauses (DSSTREAMPAUSE_*).
void IDirectSoundStream_Pause()
{
    HleCall call;
    const GuestAddress pointer = call.Arg(0);
    const uint32_t pause = call.Arg(1);
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    Stream* stream = FindStream(pointer);
    if (stream != nullptr)
    {
        stream->paused = pause != 0;
        ++stream->pause_calls;
        if (state.trace && stream->pause_calls <= kMaxLoggedCalls)
        {
            std::fprintf(stderr, "[dsound] stream %08X Pause(%u) #%llu\n", pointer, pause,
                         static_cast<unsigned long long>(stream->pause_calls));
        }
    }
    call.Return(kDsOk, 2);
}

// HRESULT IDirectSoundStream_SetVolume(LPDIRECTSOUNDSTREAM, LONG lVolume)
void IDirectSoundStream_SetVolume()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(State().mutex);
    Stream* stream = FindStream(call.Arg(0));
    if (stream != nullptr)
    {
        stream->volume = static_cast<int32_t>(call.Arg(1));
    }
    call.Return(kDsOk, 2);
}

// HRESULT IDirectSoundStream_SetHeadroom(LPDIRECTSOUNDSTREAM, DWORD dwHeadroom)
void IDirectSoundStream_SetHeadroom()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(State().mutex);
    Stream* stream = FindStream(call.Arg(0));
    if (stream != nullptr)
    {
        stream->headroom = call.Arg(1);
    }
    call.Return(kDsOk, 2);
}

// HRESULT IDirectSoundStream_SetFrequency(LPDIRECTSOUNDSTREAM, DWORD dwFrequency)
void IDirectSoundStream_SetFrequency()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(State().mutex);
    Stream* stream = FindStream(call.Arg(0));
    if (stream != nullptr)
    {
        stream->frequency = call.Arg(1);
    }
    call.Return(kDsOk, 2);
}

// HRESULT IDirectSoundStream_FlushEx(LPDIRECTSOUNDSTREAM, REFERENCE_TIME rtTimeStamp, DWORD dwFlags):
// flushes at once (the time stamp is 8 bytes).
void IDirectSoundStream_FlushEx()
{
    HleCall call;
    const GuestAddress pointer = call.Arg(0);
    {
        DsoundState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        Stream* stream = FindStream(pointer);
        if (stream != nullptr)
        {
            ++stream->flushes;
            if (state.trace && stream->flushes <= kMaxLoggedCalls)
            {
                std::fprintf(stderr, "[dsound] stream %08X FlushEx #%llu (flags %08X)\n", pointer,
                             static_cast<unsigned long long>(stream->flushes), call.Arg(3));
            }
            FlushLocked(*stream);
        }
    }
    DeliverCompletions();
    call.Return(kDsOk, 4);
}

// The guest vtable every stream object points at, made with the first stream
// (the guest heap exists by then).
void InitializeVtable()
{
    if (g_vtable != 0)
    {
        return;
    }
    g_vtable = AllocateGuest(4 * static_cast<uint32_t>(std::size(kMethodNames)));
    for (uint32_t i = 0; i < std::size(kMethodNames); ++i)
    {
        Memory().Write32(g_vtable + 4 * i, kMethodVaBase + 4 * i);
    }
}

// VOID DirectSoundDoWork(VOID): the library's periodic work; here the stream
// packets the mixer finished are completed (design i054).
void DirectSoundDoWork()
{
    HleCall call;
    DeliverCompletions();
    call.Return(call.Eax(), 0);
}

template <size_t... I>
void BindMethods(runtime::HleRegistry& registry, std::index_sequence<I...>)
{
    (registry.BindGuestAddress(kMethodVaBase + 4 * static_cast<uint32_t>(I), kMethodNames[I]), ...);
}

}  // namespace

void DeliverStreamCompletions()
{
    DeliverCompletions();
}

void MixStreamsLocked(float* samples, uint32_t frames)
{
    for (auto& entry : g_streams)
    {
        MixStream(entry.second, samples, frames);
    }
    DsoundState& state = State();
    if (!state.trace || (g_streams.empty() && state.buffers.empty()))
    {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (g_last_report.time_since_epoch().count() == 0)
    {
        g_last_report = now;
        return;
    }
    if (now - g_last_report < kReportInterval)
    {
        return;
    }
    g_last_report = now;
    for (auto& [pointer, stream] : g_streams)
    {
        uint32_t done_count = 0;
        for (const Packet& packet : stream.packets)
        {
            if (packet.done)
            {
                ++done_count;
            }
        }
        char delivery[32];
        if (stream.last_delivery.time_since_epoch().count() == 0)
        {
            std::snprintf(delivery, sizeof(delivery), "never");
        }
        else
        {
            const double ago = std::chrono::duration<double>(now - stream.last_delivery).count();
            std::snprintf(delivery, sizeof(delivery), "%.1f s ago", ago);
        }
        std::fprintf(stderr,
                     "[dsound] stream %08X: submitted %llu (%llu bytes), mixed %llu, completed %llu, queued %zu (%u "
                     "done), starved %llu, paused-mixes %llu, flushes %llu, discontinuities %llu, pauses %llu (now "
                     "%s), last delivery %s, peak raw %.3f out %.3f, volume %d, headroom %u\n",
                     pointer, static_cast<unsigned long long>(stream.submitted),
                     static_cast<unsigned long long>(stream.submitted_bytes),
                     static_cast<unsigned long long>(stream.mixed_out),
                     static_cast<unsigned long long>(stream.completed), stream.packets.size(), done_count,
                     static_cast<unsigned long long>(stream.starved_mixes),
                     static_cast<unsigned long long>(stream.paused_mixes),
                     static_cast<unsigned long long>(stream.flushes),
                     static_cast<unsigned long long>(stream.discontinuities),
                     static_cast<unsigned long long>(stream.pause_calls), stream.paused ? "paused" : "playing",
                     delivery, stream.peak_raw, stream.peak_out, stream.volume, stream.headroom);
        stream.peak_raw = 0.0f;
        stream.peak_out = 0.0f;
    }
    std::fprintf(stderr, "[dsound] output: peak %.3f, buffers playing %u (max), streams %zu\n", state.output_peak,
                 state.max_playing_buffers, g_streams.size());
    state.output_peak = 0.0f;
    state.max_playing_buffers = 0;
}

bool StreamStatsForTest(GuestAddress pointer, StreamStats* stats)
{
    std::lock_guard<std::mutex> lock(State().mutex);
    const Stream* stream = FindStream(pointer);
    if (stream == nullptr || stats == nullptr)
    {
        return false;
    }
    stats->submitted = stream->submitted;
    stats->submitted_bytes = stream->submitted_bytes;
    stats->mixed_out = stream->mixed_out;
    stats->completed = stream->completed;
    stats->starved_mixes = stream->starved_mixes;
    stats->paused_mixes = stream->paused_mixes;
    stats->flushes = stream->flushes;
    stats->discontinuities = stream->discontinuities;
    stats->pause_calls = stream->pause_calls;
    stats->queued = static_cast<uint32_t>(stream->packets.size());
    stats->done_queued = 0;
    for (const Packet& packet : stream->packets)
    {
        if (packet.done)
        {
            ++stats->done_queued;
        }
    }
    stats->paused = stream->paused;
    stats->has_delivery = stream->last_delivery.time_since_epoch().count() != 0;
    stats->peak_raw = stream->peak_raw;
    stats->peak_out = stream->peak_out;
    stats->volume = stream->volume;
    stats->headroom = stream->headroom;
    return true;
}

void RegisterStreamExports(runtime::HleRegistry& registry)
{
    registry.AddExport("dsound", "DirectSoundDoWork", &DirectSoundDoWork);
    registry.AddExport("dsound", "DirectSoundCreateStream", &DirectSoundCreateStream);
    registry.AddExport("dsound", "IDirectSound_CreateSoundStream", &IDirectSound_CreateSoundStream);
    registry.AddExport("dsound", "DSStream_AddRef", &DSStream_AddRef);
    registry.AddExport("dsound", "DSStream_Release", &DSStream_Release);
    registry.AddExport("dsound", "DSStream_GetInfo", &DSStream_GetInfo);
    registry.AddExport("dsound", "DSStream_GetStatus", &DSStream_GetStatus);
    registry.AddExport("dsound", "DSStream_Process", &DSStream_Process);
    registry.AddExport("dsound", "DSStream_Discontinuity", &DSStream_Discontinuity);
    registry.AddExport("dsound", "DSStream_Flush", &DSStream_Flush);
    registry.AddExport("dsound", "DSStream_Destructor", &DSStream_Destructor);
    BindMethods(registry, std::make_index_sequence<std::size(kMethodNames)>{});
    registry.AddExport("dsound", "IDirectSoundStream_Pause", &IDirectSoundStream_Pause);
    registry.AddExport("dsound", "IDirectSoundStream_SetVolume", &IDirectSoundStream_SetVolume);
    registry.AddExport("dsound", "IDirectSoundStream_SetHeadroom", &IDirectSoundStream_SetHeadroom);
    registry.AddExport("dsound", "IDirectSoundStream_SetFrequency", &IDirectSoundStream_SetFrequency);
    registry.AddExport("dsound", "IDirectSoundStream_FlushEx", &IDirectSoundStream_FlushEx);
    // Settings the HLE does not render yet (design i054 section 3).
    registry.AddExport("dsound", "IDirectSoundStream_SetMixBins", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetMixBinVolumes_8", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetPitch", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetFilter", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetLFO", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetEG", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetOutputBuffer", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_Set3DVoiceData", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_Use3DVoiceData", &AcceptSetting<2>);
    registry.AddExport("dsound", "IDirectSoundStream_SetFormat", &AcceptSetting<2>);
}

}  // namespace rexbox::hle::dsound
