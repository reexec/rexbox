// State blocks (issue #57, docs/design/20261103-i057-state-blocks.md): the
// library captures its device's internal fields, which the HLE device does
// not keep, so a lifted CreateStateBlock records zeros and its ApplyStateBlock
// restores a null vertex shader and a 0x0 viewport. Here a block is a host
// snapshot of the HLE's own state, keyed by a guest token.

#include "d3d8_globals.h"
#include "d3d8_internal.h"
#include "d3d8_render_state.h"
#include "d3d8_state.h"

#include <array>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace rexbox::hle::d3d8
{

namespace
{

constexpr uint32_t kSnapshotStreams = 16;

struct Snapshot
{
    std::vector<uint32_t> render_states;  // by the title's slot
    std::array<std::array<uint32_t, globals::kTextureStateSlots>, kMaxTextureStages> texture_states = {};
    std::array<GuestAddress, kSnapshotStreams> stream_buffers = {};
    std::array<uint32_t, kSnapshotStreams> stream_strides = {};
    PipelineState pipeline;
    Viewport viewport;
};

std::mutex g_mutex;
std::unordered_map<GuestAddress, Snapshot> g_blocks;

void Capture(Snapshot& snapshot)
{
    const uint32_t slots = RenderStateSlotCount(CurrentRenderStateLayout());
    snapshot.render_states.resize(slots);
    for (uint32_t slot = 0; slot < slots; ++slot)
    {
        snapshot.render_states[slot] = RenderStateSlotValue(slot);
    }
    for (uint32_t stage = 0; stage < kMaxTextureStages; ++stage)
    {
        for (uint32_t type = 0; type < globals::kTextureStateSlots; ++type)
        {
            snapshot.texture_states[stage][type] = TextureStateValue(stage, type);
        }
    }
    for (uint32_t stream = 0; stream < kSnapshotStreams; ++stream)
    {
        snapshot.stream_buffers[stream] = StreamVertexBuffer(stream);
        snapshot.stream_strides[stream] = StreamStride(stream);
    }
    snapshot.pipeline = Pipeline();
    snapshot.viewport = State().viewport;
}

void Apply(const Snapshot& snapshot)
{
    for (uint32_t slot = 0; slot < snapshot.render_states.size(); ++slot)
    {
        SetRenderStateSlot(slot, snapshot.render_states[slot]);
    }
    for (uint32_t stage = 0; stage < kMaxTextureStages; ++stage)
    {
        for (uint32_t type = 0; type < globals::kTextureStateSlots; ++type)
        {
            SetTextureStateValue(stage, type, snapshot.texture_states[stage][type]);
        }
    }
    for (uint32_t stream = 0; stream < kSnapshotStreams; ++stream)
    {
        SetStream(stream, snapshot.stream_buffers[stream], snapshot.stream_strides[stream]);
    }
    // The pipeline as captured; the texture bindings' reference counts are
    // not replayed (the HLE frees no texture on them).
    PipelineState& pipeline = Pipeline();
    const uint64_t generation = pipeline.texture_generation;
    pipeline = snapshot.pipeline;
    pipeline.texture_generation = generation + 1;
    State().viewport = snapshot.viewport;  // applied per draw (d3d8_draw)
    for (uint32_t state = 0; state < kDeviceTransforms; ++state)
    {
        StoreTransformToDevice(state);
    }
}

// HRESULT D3DDevice_CreateStateBlock(D3DSTATEBLOCKTYPE Type, DWORD* pToken):
// every type captures everything (D3DSBT_ALL; pixel- and vertex-only blocks
// are a superset here, inferred harmless).
void D3DDevice_CreateStateBlock()
{
    HleCall call;
    const GuestAddress out = call.Arg(1);
    const auto block = Heap().AllocateSmall(16);
    const GuestAddress token = block ? *block : 0;
    if (out == 0 || token == 0)
    {
        call.Return(kErrOutOfMemory, 2);
        return;
    }
    Snapshot snapshot;
    Capture(snapshot);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_blocks[token] = std::move(snapshot);
    }
    Memory().Write32(out, token);
    call.Return(kD3dOk, 2);
}

// HRESULT D3DDevice_ApplyStateBlock(DWORD Token)
void D3DDevice_ApplyStateBlock()
{
    HleCall call;
    Snapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        const auto it = g_blocks.find(call.Arg(0));
        if (it == g_blocks.end())
        {
            call.Return(kD3dErrInvalidCall, 1);
            return;
        }
        snapshot = it->second;
    }
    Apply(snapshot);
    call.Return(kD3dOk, 1);
}

// HRESULT D3DDevice_CaptureStateBlock(DWORD Token)
void D3DDevice_CaptureStateBlock()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(g_mutex);
    const auto it = g_blocks.find(call.Arg(0));
    if (it != g_blocks.end())
    {
        Capture(it->second);
    }
    call.Return(it != g_blocks.end() ? kD3dOk : kD3dErrInvalidCall, 1);
}

// HRESULT D3DDevice_DeleteStateBlock(DWORD Token)
void D3DDevice_DeleteStateBlock()
{
    HleCall call;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_blocks.erase(call.Arg(0));
    call.Return(kD3dOk, 1);
}

}  // namespace

void RegisterStateBlockExports(runtime::HleRegistry& registry)
{
    registry.AddExport("d3d8", "D3DDevice_CreateStateBlock", &D3DDevice_CreateStateBlock);
    registry.AddExport("d3d8", "D3DDevice_ApplyStateBlock", &D3DDevice_ApplyStateBlock);
    registry.AddExport("d3d8", "D3DDevice_CaptureStateBlock", &D3DDevice_CaptureStateBlock);
    registry.AddExport("d3d8", "D3DDevice_DeleteStateBlock", &D3DDevice_DeleteStateBlock);
}

}  // namespace rexbox::hle::d3d8
