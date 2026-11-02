// 3D voices: the buffer and listener 3D settings, and what the mixer takes
// from them -- distance and cone attenuation, a stereo pan and a Doppler pitch
// ratio (docs/design/20261102-i040-dsound-3d.md).
//
// The distance attenuation is the XDK 4531 library's (confirmed in its code);
// the cone and Doppler follow DirectSound 3D's definitions, and the pan is a
// balance-law stand-in for the original's HRTF filters. Settings apply at
// once, DS3D_DEFERRED ones included.

#include "dsound_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace rexbox::hle::dsound
{

namespace
{

constexpr float kSpeedOfSound = 343.3f;  // meters per second
constexpr double kMinDopplerRatio = 0.5;
constexpr double kMaxDopplerRatio = 2.0;
constexpr float kNearZero = 1e-6f;
constexpr float kPi = 3.14159265358979f;

float FloatArg(const HleCall& call, int index)
{
    const uint32_t bits = call.Arg(index);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Vector3 VectorArgs(const HleCall& call, int first)
{
    return Vector3{FloatArg(call, first), FloatArg(call, first + 1), FloatArg(call, first + 2)};
}

Vector3 Sub(const Vector3& a, const Vector3& b)
{
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

float Dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Cross(const Vector3& a, const Vector3& b)
{
    return Vector3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float Length(const Vector3& v)
{
    return std::sqrt(Dot(v, v));
}

// mB to a gain; -10000 and below is silence.
float GainOf(float hundredths_db)
{
    return hundredths_db <= -10000.0f ? 0.0f : std::pow(10.0f, hundredths_db / 2000.0f);
}

// The XDK 4531 attenuation: none within the min distance, the distance clamped
// to the max, else -2000 * log10(1 + R * (d / min - 1)) mB with R the listener
// rolloff times the buffer's.
float DistanceHundredthsDb(const Spatial& spatial, const Listener& listener, float distance)
{
    const float min_distance = std::max(spatial.min_distance, kNearZero);
    if (distance <= min_distance)
    {
        return 0.0f;
    }
    const float clamped = std::min(distance, std::max(spatial.max_distance, min_distance));
    const float rolloff = listener.rolloff * spatial.rolloff;
    const float factor = 1.0f + rolloff * (clamped / min_distance - 1.0f);
    return factor <= 1.0f ? 0.0f : -2000.0f * std::log10(factor);
}

// DirectSound 3D's cone: full volume within half the inside angle, the outside
// volume beyond half the outside angle, interpolated in mB between.
// `to_listener` is the unit vector from the source to the listener.
float ConeHundredthsDb(const Spatial& spatial, const Vector3& to_listener)
{
    if (spatial.inside_cone_angle >= 360 && spatial.outside_cone_angle >= 360)
    {
        return 0.0f;
    }
    const float length = Length(spatial.cone_orientation);
    if (length < kNearZero)
    {
        return 0.0f;
    }
    const float cosine = std::clamp(Dot(spatial.cone_orientation, to_listener) / length, -1.0f, 1.0f);
    const float angle = std::acos(cosine) * 180.0f / kPi;
    const float inside = std::min(spatial.inside_cone_angle, 360u) * 0.5f;
    const float outside = std::max(inside, std::min(spatial.outside_cone_angle, 360u) * 0.5f);
    if (angle <= inside)
    {
        return 0.0f;
    }
    const float outside_volume = static_cast<float>(spatial.cone_outside_volume);
    if (angle >= outside || outside - inside < kNearZero)
    {
        return outside_volume;
    }
    return outside_volume * (angle - inside) / (outside - inside);
}

template <typename Apply>
void SetBuffer3d(int stack_args, Apply apply)
{
    HleCall call;
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    Buffer* buffer = FindBuffer(call.Arg(0));
    if (buffer == nullptr)
    {
        call.Return(kErrInvalidCall, stack_args);
        return;
    }
    apply(call, buffer->spatial);
    Update3d(*buffer);
    call.Return(kDsOk, stack_args);
}

template <typename Apply>
void SetListener(int stack_args, Apply apply)
{
    HleCall call;
    DsoundState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    apply(call, state.listener);
    Update3dAll();
    call.Return(kDsOk, stack_args);
}

// HRESULT IDirectSoundBuffer_SetPosition(pThis, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
void IDirectSoundBuffer_SetPosition()
{
    SetBuffer3d(5, [](const HleCall& call, Spatial& s) { s.position = VectorArgs(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetVelocity(pThis, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
void IDirectSoundBuffer_SetVelocity()
{
    SetBuffer3d(5, [](const HleCall& call, Spatial& s) { s.velocity = VectorArgs(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetConeOrientation(pThis, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
void IDirectSoundBuffer_SetConeOrientation()
{
    SetBuffer3d(5, [](const HleCall& call, Spatial& s) { s.cone_orientation = VectorArgs(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetConeAngles(pThis, DWORD dwInsideConeAngle,
//     DWORD dwOutsideConeAngle, DWORD dwApply)
void IDirectSoundBuffer_SetConeAngles()
{
    SetBuffer3d(4, [](const HleCall& call, Spatial& s) {
        s.inside_cone_angle = call.Arg(1);
        s.outside_cone_angle = call.Arg(2);
    });
}

// HRESULT IDirectSoundBuffer_SetConeOutsideVolume(pThis, LONG lConeOutsideVolume, DWORD dwApply)
void IDirectSoundBuffer_SetConeOutsideVolume()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.cone_outside_volume = static_cast<int32_t>(call.Arg(1)); });
}

// HRESULT IDirectSoundBuffer_SetMinDistance(pThis, FLOAT flMinDistance, DWORD dwApply)
void IDirectSoundBuffer_SetMinDistance()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.min_distance = FloatArg(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetMaxDistance(pThis, FLOAT flMaxDistance, DWORD dwApply)
void IDirectSoundBuffer_SetMaxDistance()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.max_distance = FloatArg(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetMode(pThis, DWORD dwMode, DWORD dwApply)
void IDirectSoundBuffer_SetMode()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.mode = call.Arg(1); });
}

// HRESULT IDirectSoundBuffer_SetRolloffFactor(pThis, FLOAT flRolloffFactor, DWORD dwApply)
void IDirectSoundBuffer_SetRolloffFactor()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.rolloff = FloatArg(call, 1); });
}

// HRESULT IDirectSoundBuffer_SetDopplerFactor(pThis, FLOAT flDopplerFactor, DWORD dwApply)
void IDirectSoundBuffer_SetDopplerFactor()
{
    SetBuffer3d(3, [](const HleCall& call, Spatial& s) { s.doppler = FloatArg(call, 1); });
}

// HRESULT IDirectSound_SetPosition(pThis, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
void IDirectSound_SetPosition()
{
    SetListener(5, [](const HleCall& call, Listener& l) { l.position = VectorArgs(call, 1); });
}

// HRESULT IDirectSound_SetVelocity(pThis, FLOAT x, FLOAT y, FLOAT z, DWORD dwApply)
void IDirectSound_SetVelocity()
{
    SetListener(5, [](const HleCall& call, Listener& l) { l.velocity = VectorArgs(call, 1); });
}

// HRESULT IDirectSound_SetOrientation(pThis, FLOAT xFront, FLOAT yFront, FLOAT zFront,
//     FLOAT xTop, FLOAT yTop, FLOAT zTop, DWORD dwApply)
void IDirectSound_SetOrientation()
{
    SetListener(8, [](const HleCall& call, Listener& l) {
        l.front = VectorArgs(call, 1);
        l.top = VectorArgs(call, 4);
    });
}

// HRESULT IDirectSound_SetDistanceFactor(pThis, FLOAT flDistanceFactor, DWORD dwApply)
void IDirectSound_SetDistanceFactor()
{
    SetListener(3, [](const HleCall& call, Listener& l) { l.distance_factor = FloatArg(call, 1); });
}

// HRESULT IDirectSound_SetDopplerFactor(pThis, FLOAT flDopplerFactor, DWORD dwApply)
void IDirectSound_SetDopplerFactor()
{
    SetListener(3, [](const HleCall& call, Listener& l) { l.doppler = FloatArg(call, 1); });
}

// HRESULT IDirectSound_SetRolloffFactor(pThis, FLOAT flRolloffFactor, DWORD dwApply)
void IDirectSound_SetRolloffFactor()
{
    SetListener(3, [](const HleCall& call, Listener& l) { l.rolloff = FloatArg(call, 1); });
}

// HRESULT IDirectSound_CommitDeferredSettings(pThis): settings already apply at once.
void IDirectSound_CommitDeferredSettings()
{
    SetListener(1, [](const HleCall&, Listener&) {});
}

}  // namespace

bool Is3dActive(const Buffer& buffer)
{
    return (buffer.flags & kCaps3d) != 0 && buffer.spatial.mode != kMode3dDisable;
}

void Update3d(Buffer& buffer)
{
    Spatial& s = buffer.spatial;
    s.attenuation = 1.0f;
    s.pan_left = 1.0f;
    s.pan_right = 1.0f;
    s.doppler_ratio = 1.0;
    if (!Is3dActive(buffer))
    {
        UpdateGains(buffer);
        return;
    }
    const Listener& listener = State().listener;
    const bool head_relative = s.mode == kMode3dHeadRelative;
    // The source relative to the listener, and the listener's right axis
    // (left-handed: top x front), both in the same space. A degenerate
    // orientation (zero or parallel vectors) falls back to the default axes.
    const Vector3 relative = head_relative ? s.position : Sub(s.position, listener.position);
    Vector3 right = head_relative ? Vector3{1.0f, 0.0f, 0.0f} : Cross(listener.top, listener.front);
    if (Length(right) < kNearZero)
    {
        right = Vector3{1.0f, 0.0f, 0.0f};
    }
    const float distance = Length(relative);

    float hundredths_db = DistanceHundredthsDb(s, listener, distance);
    if (distance > kNearZero)
    {
        const Vector3 to_source{relative.x / distance, relative.y / distance, relative.z / distance};
        hundredths_db += ConeHundredthsDb(s, Vector3{-to_source.x, -to_source.y, -to_source.z});

        const float pan = std::clamp(Dot(to_source, right) / Length(right), -1.0f, 1.0f);
        s.pan_left = std::min(1.0f, 1.0f - pan);
        s.pan_right = std::min(1.0f, 1.0f + pan);

        // f' = f (c + vL.u) / (c + vS.u), u from the listener to the source.
        const float factor = listener.doppler * s.doppler;
        const float meters_per_unit = listener.distance_factor > kNearZero ? listener.distance_factor : 1.0f;
        const double speed = kSpeedOfSound / meters_per_unit;
        if (factor > 0.0f)
        {
            const Vector3 listener_velocity = head_relative ? Vector3{} : listener.velocity;
            const double toward_listener = factor * Dot(listener_velocity, to_source);
            const double away_source = factor * Dot(s.velocity, to_source);
            const double denominator = speed + away_source;
            if (denominator > 0.0)
            {
                s.doppler_ratio = std::clamp((speed + toward_listener) / denominator, kMinDopplerRatio, kMaxDopplerRatio);
            }
            else
            {
                s.doppler_ratio = kMaxDopplerRatio;
            }
        }
    }
    s.attenuation = GainOf(hundredths_db);
    UpdateGains(buffer);
}

void Update3dAll()
{
    for (auto& entry : State().buffers)
    {
        if ((entry.second.flags & kCaps3d) != 0)
        {
            Update3d(entry.second);
        }
    }
}

void Register3dExports(runtime::HleRegistry& registry)
{
    registry.AddExport("dsound", "IDirectSoundBuffer_SetPosition", &IDirectSoundBuffer_SetPosition);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetVelocity", &IDirectSoundBuffer_SetVelocity);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetConeOrientation", &IDirectSoundBuffer_SetConeOrientation);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetConeAngles", &IDirectSoundBuffer_SetConeAngles);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetConeOutsideVolume", &IDirectSoundBuffer_SetConeOutsideVolume);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetMinDistance", &IDirectSoundBuffer_SetMinDistance);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetMaxDistance", &IDirectSoundBuffer_SetMaxDistance);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetMode", &IDirectSoundBuffer_SetMode);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetRolloffFactor", &IDirectSoundBuffer_SetRolloffFactor);
    registry.AddExport("dsound", "IDirectSoundBuffer_SetDopplerFactor", &IDirectSoundBuffer_SetDopplerFactor);
    registry.AddExport("dsound", "IDirectSound_SetPosition", &IDirectSound_SetPosition);
    registry.AddExport("dsound", "IDirectSound_SetVelocity", &IDirectSound_SetVelocity);
    registry.AddExport("dsound", "IDirectSound_SetOrientation", &IDirectSound_SetOrientation);
    registry.AddExport("dsound", "IDirectSound_SetDistanceFactor", &IDirectSound_SetDistanceFactor);
    registry.AddExport("dsound", "IDirectSound_SetDopplerFactor", &IDirectSound_SetDopplerFactor);
    registry.AddExport("dsound", "IDirectSound_SetRolloffFactor", &IDirectSound_SetRolloffFactor);
    registry.AddExport("dsound", "IDirectSound_CommitDeferredSettings", &IDirectSound_CommitDeferredSettings);
}

}  // namespace rexbox::hle::dsound
