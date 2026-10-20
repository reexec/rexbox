#include "d3d8_push.h"

#include "d3d8_internal.h"
#include "d3d8_trace.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace rexbox::hle::d3d8
{

namespace
{

// NV2A (Kelvin) methods the interpreter understands.
constexpr uint32_t kMethodBeginEnd = 0x17FC;     // NV097_SET_BEGIN_END: primitive, 0 ends
constexpr uint32_t kMethodInlineArray = 0x1818;  // NV097_INLINE_ARRAY: vertex DWORDs

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
    uint32_t primitive = 0;
    std::vector<uint32_t> vertices;
};

PushState g_push;

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
        }
        else
        {
            EndPrimitive();
        }
        break;
    case kMethodInlineArray:
        g_push.vertices.push_back(value);
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

}  // namespace

void RegisterPushExports(runtime::HleRegistry& registry)
{
    registry.AddExport("d3d8", "D3DDevice_BeginPush_4", &D3DDevice_BeginPush_4);
    registry.AddExport("d3d8", "D3DDevice_EndPush", &D3DDevice_EndPush);
}

}  // namespace rexbox::hle::d3d8
