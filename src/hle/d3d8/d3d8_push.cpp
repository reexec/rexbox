#include "d3d8_push.h"

#include "d3d8_internal.h"
#include "d3d8_trace.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace rexbox::hle::d3d8
{

namespace
{

// NV2A (Kelvin) methods the interpreter understands.
constexpr uint32_t kMethodBeginEnd = 0x17FC;     // NV097_SET_BEGIN_END: primitive, 0 ends
constexpr uint32_t kMethodInlineArray = 0x1818;  // NV097_INLINE_ARRAY: vertex DWORDs
constexpr uint32_t kMethodArrayElement16 = 0x1800;  // NV097_ARRAY_ELEMENT16: two 16-bit indices
constexpr uint32_t kMethodArrayElement32 = 0x1808;  // NV097_ARRAY_ELEMENT32: one 32-bit index

// Push-buffer header fields.
constexpr uint32_t kHeaderMethodMask = 0x1FFC;
constexpr uint32_t kHeaderCountShift = 18;
constexpr uint32_t kHeaderCountMask = 0x7FF;
constexpr uint32_t kHeaderNonIncrement = 0x40000000;
constexpr uint32_t kHeaderOldJumpMask = 0xE0000003;
constexpr uint32_t kHeaderOldJump = 0x20000000;
constexpr uint32_t kHeaderKindMask = 0x3;  // 1 jump, 2 call
constexpr uint32_t kHeaderReturn = 0x00020000;

// The scratch buffer BeginPush hands out (valid until the next BeginPush),
// the staging buffer inline vertices are gathered into for Draw, and the
// primitive between SET_BEGIN_END calls. One drawing thread is assumed.
struct PushState
{
    GuestAddress buffer = 0;
    uint32_t buffer_bytes = 0;
    GuestAddress staging = 0;
    uint32_t staging_bytes = 0;
    GuestAddress index_staging = 0;
    uint32_t index_staging_bytes = 0;
    uint32_t primitive = 0;
    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indices;  // ARRAY_ELEMENT16/32 into the current vertex arrays
};

PushState g_push;

// The device push buffer (design i051). The device object's fields are the
// title's view; `flushed` is how far the HLE has interpreted.
constexpr uint32_t kDevicePushBytes = 0x80000;     // the library's default size
constexpr uint32_t kDevicePushMargin = 0x4000;     // room past the threshold
constexpr uint32_t kDevicePut = 0x00;              // write pointer
constexpr uint32_t kDeviceThreshold = 0x04;        // make room once pPut reaches it
constexpr uint32_t kDeviceFlags = 0x08;            // bit 2: recording (not handled)
constexpr uint32_t kDeviceBase = 0x24;
constexpr uint32_t kDeviceEnd = 0x28;
constexpr uint32_t kDeviceFlagRecording = 0x4;

struct DevicePush
{
    GuestAddress device = 0;
    GuestAddress base = 0;
    GuestAddress end = 0;
    GuestAddress flushed = 0;
};

DevicePush g_device_push;

// Makes `*block` hold at least `bytes`, reallocating page-granular guest heap
// memory when it is smaller (the old contents are not kept).
bool Reserve(GuestAddress* block, uint32_t* capacity, uint32_t bytes)
{
    if (bytes <= *capacity)
    {
        return true;
    }
    const uint32_t page = runtime::GuestHeap::kPageSize;
    const uint32_t size = (bytes + page - 1) & ~(page - 1);
    const auto allocated = Heap().Allocate(size, page, false);
    if (!allocated)
    {
        return false;
    }
    if (*block != 0)
    {
        Heap().Free(*block);
    }
    *block = *allocated;
    *capacity = size;
    return true;
}

void EndPrimitive()
{
    if (g_push.primitive != 0 && !g_push.indices.empty())
    {
        const uint32_t bytes = static_cast<uint32_t>(g_push.indices.size() * 2);
        if (Reserve(&g_push.index_staging, &g_push.index_staging_bytes, bytes))
        {
            std::memcpy(Memory().Translate(g_push.index_staging), g_push.indices.data(), bytes);
            DrawArrayElements(g_push.primitive, g_push.index_staging, static_cast<uint32_t>(g_push.indices.size()));
        }
    }
    if (g_push.primitive != 0 && !g_push.vertices.empty())
    {
        const uint32_t bytes = static_cast<uint32_t>(g_push.vertices.size() * 4);
        if (Reserve(&g_push.staging, &g_push.staging_bytes, bytes))
        {
            std::memcpy(Memory().Translate(g_push.staging), g_push.vertices.data(), bytes);
            DrawInlineVertices(g_push.primitive, g_push.staging, bytes);
        }
    }
    g_push.primitive = 0;
    g_push.vertices.clear();
    g_push.indices.clear();
}

void Method(uint32_t method, uint32_t value)
{
    switch (method)
    {
    case kMethodBeginEnd:
        if (value != 0)
        {
            g_push.primitive = value;
            g_push.vertices.clear();
            g_push.indices.clear();
        }
        else
        {
            EndPrimitive();
        }
        break;
    case kMethodInlineArray:
        g_push.vertices.push_back(value);
        break;
    case kMethodArrayElement16:
        g_push.indices.push_back(static_cast<uint16_t>(value));
        g_push.indices.push_back(static_cast<uint16_t>(value >> 16));
        break;
    case kMethodArrayElement32:
        if (value > 0xFFFF)
        {
            ReportOncePer("push buffer index", 0, [&] {
                return Format("push buffer ARRAY_ELEMENT32 index %08X is past 16 bits; skipped", value);
            });
            break;
        }
        g_push.indices.push_back(static_cast<uint16_t>(value));
        break;
    default:
        ReportOncePer("push buffer method", method, [&] {
            return Format("push buffer method %04X (first value %08X) is not interpreted; skipped", method, value);
        });
        break;
    }
}

// Interprets the method headers and values in [begin, end).
void Execute(GuestAddress begin, GuestAddress end)
{
    runtime::GuestMemory& memory = Memory();
    GuestAddress at = begin;
    while (at + 4 <= end)
    {
        const uint32_t header = memory.Read32(at);
        at += 4;
        if ((header & kHeaderOldJumpMask) == kHeaderOldJump || (header & kHeaderKindMask) != 0 ||
            header == kHeaderReturn)
        {
            ReportOncePer("push buffer control", header & kHeaderOldJumpMask, [&] {
                return Format("push buffer jump, call or return %08X is not supported; the rest is skipped", header);
            });
            return;
        }
        const uint32_t method = header & kHeaderMethodMask;
        const uint32_t count = (header >> kHeaderCountShift) & kHeaderCountMask;
        const bool increment = (header & kHeaderNonIncrement) == 0;
        for (uint32_t i = 0; i < count && at + 4 <= end; ++i)
        {
            Method(increment ? method + i * 4 : method, memory.Read32(at));
            at += 4;
        }
    }
}

// DWORD* D3DDevice_BeginPush(DWORD Count) (XDK 4531, `BeginPush_4`): room for
// Count DWORDs the title fills with NV2A methods.
void D3DDevice_BeginPush_4()
{
    HleCall call;
    FlushDevicePushBuffer();
    const uint32_t count = std::max<uint32_t>(call.Arg(0), 1);
    if (!Reserve(&g_push.buffer, &g_push.buffer_bytes, count * 4))
    {
        call.Return(0, 1);
        return;
    }
    call.Return(g_push.buffer, 1);
}

// void D3DDevice_EndPush(DWORD* pPush): pPush is one past the last DWORD written.
void D3DDevice_EndPush()
{
    HleCall call;
    const GuestAddress end = call.Arg(0);
    if (g_push.buffer != 0 && end >= g_push.buffer && end <= g_push.buffer + g_push.buffer_bytes)
    {
        Execute(g_push.buffer, end);
    }
    call.Return(kD3dOk, 1);
}

// Puts the device's write pointer back at the start of its buffer, once what
// was written there is interpreted. Returns the new pPut.
GuestAddress RewindDevicePushBuffer()
{
    FlushDevicePushBuffer();
    if (g_device_push.device == 0)
    {
        return 0;
    }
    if ((Memory().Read8(g_device_push.device + kDeviceFlags) & kDeviceFlagRecording) != 0)
    {
        ReportOncePer("push buffer recording", 0, [] {
            return std::string("device push buffer: recording (device +0x08 bit 2) is not supported");
        });
    }
    Memory().Write32(g_device_push.device + kDevicePut, g_device_push.base);
    g_device_push.flushed = g_device_push.base;
    return g_device_push.base;
}

// DWORD* D3D_MakeRequestedSpace(DWORD MinimumSpace, DWORD RequestedSpace = eax)
// (XDK 5849 LTCG, `_4__LTCG_eax2`, ret 4): room for at least MinimumSpace
// bytes. The library waits for the GPU and wraps; here the buffer is
// interpreted and rewound (design i051).
void D3D_MakeRequestedSpace_4__LTCG_eax2()
{
    HleCall call;
    const uint32_t minimum = call.Arg(0);
    if (minimum > kDevicePushBytes - kDevicePushMargin)
    {
        ReportOncePer("push buffer request", 0, [&] {
            return Format("device push buffer: request of %08X bytes is larger than the buffer", minimum);
        });
    }
    call.Return(RewindDevicePushBuffer(), 1);
}

// void CDevice_KickOff() (`_0__LTCG_edx1`): hands the pushed methods to the
// GPU; here they are interpreted.
void CDevice_KickOff_0__LTCG_edx1()
{
    HleCall call;
    FlushDevicePushBuffer();
    call.Return(call.Eax(), 0);
}

}  // namespace

void SetupDevicePushBuffer(GuestAddress device)
{
    runtime::GuestMemory& memory = Memory();
    if (g_device_push.base == 0)
    {
        const auto buffer = Heap().Allocate(kDevicePushBytes, runtime::GuestHeap::kPageSize, false);
        if (!buffer)
        {
            std::fprintf(stderr, "[d3d8] no guest memory for the device push buffer\n");
            return;
        }
        g_device_push.base = *buffer;
        g_device_push.end = *buffer + kDevicePushBytes;
        memory.Fill(*buffer, 0, kDevicePushBytes);
    }
    g_device_push.device = device;
    g_device_push.flushed = g_device_push.base;
    memory.Write32(device + kDevicePut, g_device_push.base);
    memory.Write32(device + kDeviceThreshold, g_device_push.end - kDevicePushMargin);
    memory.Write32(device + kDeviceBase, g_device_push.base);
    memory.Write32(device + kDeviceEnd, g_device_push.end);
}

void FlushDevicePushBuffer()
{
    if (g_device_push.device == 0)
    {
        return;
    }
    const GuestAddress put = Memory().Read32(g_device_push.device + kDevicePut);
    if (put < g_device_push.flushed || put > g_device_push.end)
    {
        ReportOncePer("push buffer put", 0, [&] {
            return Format("device push buffer: pPut %08X is outside [%08X, %08X]; realigned", put,
                          g_device_push.flushed, g_device_push.end);
        });
        g_device_push.flushed = put >= g_device_push.base && put <= g_device_push.end ? put : g_device_push.flushed;
        return;
    }
    const GuestAddress begin = g_device_push.flushed;
    g_device_push.flushed = put;  // before Execute: a draw inside does not re-enter this range
    Execute(begin, put);
}

void RegisterPushExports(runtime::HleRegistry& registry)
{
    registry.AddExport("d3d8", "D3D_MakeRequestedSpace_4__LTCG_eax2", &D3D_MakeRequestedSpace_4__LTCG_eax2);
    registry.AddExport("d3d8", "CDevice_KickOff_0__LTCG_edx1", &CDevice_KickOff_0__LTCG_edx1);
    registry.AddExport("d3d8", "D3DDevice_BeginPush_4", &D3DDevice_BeginPush_4);
    registry.AddExport("d3d8", "D3DDevice_EndPush", &D3DDevice_EndPush);
}

}  // namespace rexbox::hle::d3d8
