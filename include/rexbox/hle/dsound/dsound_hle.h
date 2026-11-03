// HLE module: DirectSound (DSOUND): sound buffers mixed on the platform audio
// output (docs/design/20261012-i017-dsound-hle.md).
//
// The library is statically linked into each title, so its functions are bound
// by symbol name to title-specific guest addresses through a title profile
// (see rexbox/runtime/hle_registry.h).

#ifndef REXBOX_HLE_DSOUND_DSOUND_HLE_H
#define REXBOX_HLE_DSOUND_DSOUND_HLE_H

#include <cstdint>

namespace rexbox::runtime
{
class HleRegistry;
}

namespace rexbox::hle::dsound
{

// The rate the mixer renders at (the APU's rate).
constexpr uint32_t kOutputRate = 48000;

enum class DsoundOutput
{
    kHost,    // the platform audio output (a host device when there is one)
    kNull,    // the platform output's pacing thread only (REXBOX_AUDIO=none)
    kManual,  // nothing runs the mixer; tools call RenderDsoundFrames
};

void RegisterDsoundHle(runtime::HleRegistry& registry);

// Chooses the output; takes effect at the first DirectSoundCreate.
void SetDsoundOutput(DsoundOutput output);

// With kManual output: mixes `frames` stereo frames (2 floats each) into
// `samples` (overwritten; may be null) and advances every playing buffer.
void RenderDsoundFrames(float* samples, uint32_t frames);

}  // namespace rexbox::hle::dsound

// The APU's global sample counter (NV_PAPU XGSCNT at 0xFE80200C on the
// console, 48 kHz, inferred): samples at kOutputRate since the module started,
// wrapping at 32 bits (issue #55). rexbox maps no MMIO, so a title whose
// lifted code reads the register directly reads this through an adapter. C
// linkage so decompiled title code (C) can declare it without this header.
extern "C" uint32_t rexbox_dsound_apu_sample_count(void);

#endif  // REXBOX_HLE_DSOUND_DSOUND_HLE_H
