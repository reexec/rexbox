// The host mixer: decodes playing buffers straight from guest memory (the
// original Unlock does nothing, so the game writes into buffers as they play),
// resamples them to the output rate, folds mix bins onto stereo, and advances
// the play cursors by what it rendered.

#include "dsound_internal.h"
#include "rexbox/platform/audio.h"
#include "rexbox/runtime/thread_activity.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>
#include <thread>

namespace rexbox::hle::dsound
{

namespace
{


// IMA ADPCM step sizes and index adjustments.
constexpr int16_t kAdpcmSteps[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,   598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
constexpr int8_t kAdpcmIndexAdjust[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

float VolumeGain(int32_t hundredths_db)
{
    if (hundredths_db <= -10000)
    {
        return 0.0f;
    }
    if (hundredths_db >= 0)
    {
        return 1.0f;
    }
    return std::pow(10.0f, static_cast<float>(hundredths_db) / 2000.0f);
}

uint32_t BlockAlign(const Buffer& buffer)
{
    return std::max<uint32_t>(1, buffer.format.block_align);
}

bool IsAdpcm(const Buffer& buffer)
{
    return buffer.format.tag == kFormatXboxAdpcm;
}

int DecodeNibble(int nibble, int& predictor, int& index)
{
    const int step = kAdpcmSteps[index];
    int diff = step >> 3;
    if ((nibble & 4) != 0)
    {
        diff += step;
    }
    if ((nibble & 2) != 0)
    {
        diff += step >> 1;
    }
    if ((nibble & 1) != 0)
    {
        diff += step >> 2;
    }
    predictor += (nibble & 8) != 0 ? -diff : diff;
    predictor = std::clamp(predictor, -32768, 32767);
    index = std::clamp(index + kAdpcmIndexAdjust[nibble], 0, 88);
    return predictor;
}

// Decodes ADPCM block `block` into buffer.decoded (frames interleaved by
// channel). Each channel's block starts with {int16 sample, uint8 step index,
// uint8 reserved}; the nibbles follow in 4-byte groups, channels taking turns,
// low nibble first. The header sample is frame 0.
void DecodeAdpcmBlock(Buffer& buffer, uint32_t block)
{
    const uint32_t channels = buffer.format.channels;
    const uint8_t* in = Memory().Translate(buffer.data + block * BlockAlign(buffer));
    int predictor[kMaxChannels] = {};
    int index[kMaxChannels] = {};
    for (uint32_t c = 0; c < channels; ++c)
    {
        predictor[c] = static_cast<int16_t>(in[c * 4] | (in[c * 4 + 1] << 8));
        index[c] = std::min<int>(in[c * 4 + 2], 88);
        buffer.decoded[c] = static_cast<int16_t>(predictor[c]);
    }
    const uint8_t* nibbles = in + 4 * channels;
    for (uint32_t group = 0; group < 8; ++group)
    {
        for (uint32_t c = 0; c < channels; ++c)
        {
            const uint8_t* bytes = nibbles + (group * channels + c) * 4;
            for (uint32_t k = 0; k < 8; ++k)
            {
                const uint32_t frame = 1 + group * 8 + k;
                const int nibble = (k & 1) == 0 ? (bytes[k / 2] & 0x0F) : (bytes[k / 2] >> 4);
                const int sample = DecodeNibble(nibble, predictor[c], index[c]);
                if (frame < kAdpcmSamplesPerBlock)
                {
                    buffer.decoded[frame * channels + c] = static_cast<int16_t>(sample);
                }
            }
        }
    }
    buffer.decoded_block = block;
}

float SampleAt(Buffer& buffer, uint32_t frame, uint32_t channel)
{
    if (IsAdpcm(buffer))
    {
        const uint32_t block = frame / kAdpcmSamplesPerBlock;
        if (buffer.decoded_block != static_cast<int64_t>(block))
        {
            DecodeAdpcmBlock(buffer, block);
        }
        const uint32_t index = (frame % kAdpcmSamplesPerBlock) * buffer.format.channels + channel;
        return static_cast<float>(buffer.decoded[index]) / 32768.0f;
    }
    const uint8_t* at = Memory().Translate(buffer.data + frame * BlockAlign(buffer));
    if (buffer.format.bits == 8)
    {
        return (static_cast<float>(at[channel]) - 128.0f) / 128.0f;
    }
    int16_t value = 0;
    std::memcpy(&value, at + channel * 2, 2);
    return static_cast<float>(value) / 32768.0f;
}

// Adds a mix bin's contribution for one channel of the voice, as power: the
// speakers of one side sum their energy, as a room does, the center feeds each
// side half its power, and the LFE is left out of the stereo fold-down
// (design i040 section 3). UpdateGains takes the square roots.
void AddBin(Buffer& buffer, uint32_t channel, uint32_t bin, float gain)
{
    const float power = gain * gain;
    switch (bin)
    {
    case 0:  // FRONT_LEFT
    case 4:  // BACK_LEFT
    case 6:  // XTLK_FRONT_LEFT
    case 8:  // XTLK_BACK_LEFT
        buffer.gain_left[channel] += power;
        break;
    case 1:  // FRONT_RIGHT
    case 5:  // BACK_RIGHT
    case 7:  // XTLK_FRONT_RIGHT
    case 9:  // XTLK_BACK_RIGHT
        buffer.gain_right[channel] += power;
        break;
    case 2:  // FRONT_CENTER
        buffer.gain_left[channel] += 0.5f * power;
        buffer.gain_right[channel] += 0.5f * power;
        break;
    default:  // LOW_FREQUENCY; I3DL2 and FXSEND_n feed the effects DSP, which does not run
        break;
    }
}

// A streamed loop buffer that stops being refilled replays its last stretch
// for ever (design i043). Only buffers of this size or more are watched, and
// only once their content has changed between laps (a static loop, such as an
// engine sound, never does).
constexpr uint32_t kStreamMinBytes = 16384;
constexpr uint32_t kStreamMinChanges = 2;
constexpr uint32_t kStalledLaps = 8;
constexpr double kStalledSeconds = 2.0;

uint64_t ContentHash(const Buffer& buffer)
{
    const uint8_t* bytes = Memory().Translate(buffer.data);
    uint64_t hash = 1469598103934665603ull;  // FNV-1a
    for (uint32_t i = 0; i < buffer.bytes; ++i)
    {
        hash = (hash ^ bytes[i]) * 1099511628211ull;
    }
    return hash;
}

// Whether every frame holds the first frame's bytes (a writer filling silence).
bool ConstantContent(const Buffer& buffer)
{
    const uint8_t* bytes = Memory().Translate(buffer.data);
    const uint32_t frame = std::max<uint32_t>(1, std::min<uint32_t>(BlockAlign(buffer), 16));
    for (uint32_t i = frame; i + frame <= buffer.bytes; i += frame)
    {
        if (std::memcmp(bytes, bytes + i, frame) != 0)
        {
            return false;
        }
    }
    return true;
}

// Called when a looping buffer wraps: reports, once per stall, a streamed
// buffer playing the same non-silent content for kStalledLaps laps and
// kStalledSeconds, with the guest threads (from another thread, so the
// audio callback does not wait); and the end of such a stall.
void NoteLap(GuestAddress key, Buffer& buffer, uint32_t loop_frames, uint32_t rate)
{
    if (buffer.bytes < kStreamMinBytes || buffer.data == 0 || rate == 0)
    {
        return;
    }
    const uint64_t hash = ContentHash(buffer);
    if (hash != buffer.lap_hash)
    {
        if (buffer.stall_reported)
        {
            std::fprintf(stderr, "[dsound] buffer %08X is written again after %u unchanged lap(s)\n", key,
                         buffer.unchanged_laps);
        }
        buffer.lap_hash = hash;
        ++buffer.lap_changes;
        buffer.unchanged_laps = 0;
        buffer.stall_reported = false;
        return;
    }
    ++buffer.unchanged_laps;
    const double lap_seconds = static_cast<double>(loop_frames) / rate;
    if (buffer.stall_reported || buffer.lap_changes < kStreamMinChanges || buffer.unchanged_laps < kStalledLaps ||
        buffer.unchanged_laps * lap_seconds < kStalledSeconds || ConstantContent(buffer))
    {
        return;
    }
    buffer.stall_reported = true;
    char why[200];
    std::snprintf(why, sizeof(why),
                  "buffer %08X (%u bytes, %u Hz, %u ch) has replayed the same %.2f s for %u laps: its writer stopped",
                  key, buffer.bytes, rate, buffer.format.channels, lap_seconds, buffer.unchanged_laps);
    std::fprintf(stderr, "[dsound] %s\n", why);
    try
    {
        std::thread([text = std::string(why)] { runtime::ReportGuestThreads("[dsound]", text.c_str()); }).detach();
    }
    catch (const std::system_error&)
    {
        // No thread for the report: the line above stands alone.
    }
}

void MixBuffer(GuestAddress key, Buffer& buffer, float* samples, uint32_t frames)
{
    if (!buffer.playing || !buffer.valid_format || buffer.data == 0)
    {
        return;
    }
    const uint32_t total = TotalFrames(buffer);
    if (total == 0)
    {
        return;
    }
    uint32_t loop_begin = 0;
    uint32_t loop_end = total;
    if (buffer.looping)
    {
        loop_begin = std::min(FramesFromBytes(buffer, buffer.loop_start), total - 1);
        if (buffer.loop_length != 0)
        {
            loop_end = std::min<uint32_t>(loop_begin + std::max<uint32_t>(1, FramesFromBytes(buffer, buffer.loop_length)),
                                          total);
        }
    }
    const uint32_t rate = buffer.frequency != 0 ? buffer.frequency : buffer.format.rate;
    const double doppler = Is3dActive(buffer) ? buffer.spatial.doppler_ratio : 1.0;
    const double step = static_cast<double>(rate) * doppler / static_cast<double>(kOutputRate);
    const uint32_t channels = buffer.format.channels;
    const uint32_t end = buffer.looping ? loop_end : total;
    double position = buffer.position;
    for (uint32_t i = 0; i < frames; ++i)
    {
        if (buffer.looping)
        {
            while (position >= loop_end)
            {
                position -= static_cast<double>(loop_end - loop_begin);
            }
        }
        else if (position >= total)
        {
            // A one-shot voice stops at the end; the saved position Play
            // zeroed is what GetCurrentPosition then reports.
            buffer.playing = false;
            buffer.saved_bytes = 0;
            position = 0.0;
            break;
        }
        const uint32_t frame = static_cast<uint32_t>(position);
        const float fraction = static_cast<float>(position - frame);
        uint32_t next = frame + 1;
        if (next >= end)
        {
            next = buffer.looping ? loop_begin : frame;
        }
        float left = 0.0f;
        float right = 0.0f;
        for (uint32_t c = 0; c < channels; ++c)
        {
            const float a = SampleAt(buffer, frame, c);
            const float b = SampleAt(buffer, next, c);
            const float value = a + (b - a) * fraction;
            left += value * buffer.gain_left[c];
            right += value * buffer.gain_right[c];
        }
        samples[2 * i] += left;
        samples[2 * i + 1] += right;
        position += step;
    }
    if (buffer.playing)
    {
        if (buffer.looping)
        {
            while (position >= loop_end)
            {
                position -= static_cast<double>(loop_end - loop_begin);
            }
        }
        else if (position >= total)
        {
            buffer.playing = false;
            buffer.saved_bytes = 0;
            position = 0.0;
        }
    }
    if (buffer.playing && buffer.looping && position < buffer.position)
    {
        NoteLap(key, buffer, loop_end - loop_begin, rate);
    }
    buffer.position = position;
}

// Output WAV dump (design i072): opened in StartOutputLocked, written on the
// audio thread in RenderOutput.
std::FILE* g_dump_file = nullptr;
uint64_t g_dump_bytes = 0;
uint64_t g_dump_last_header_bytes = 0;
bool g_dump_error_reported = false;

constexpr uint32_t kWavHeaderBytes = 44;
constexpr uint64_t kDumpHeaderRefreshBytes = static_cast<uint64_t>(kOutputRate) * 4;  // 1 second of stereo 16-bit PCM

void WriteLe16(uint8_t* out, uint16_t value)
{
    out[0] = static_cast<uint8_t>(value & 0xFF);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void WriteLe32(uint8_t* out, uint32_t value)
{
    out[0] = static_cast<uint8_t>(value & 0xFF);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void BuildWavHeader(uint32_t data_bytes, uint8_t header[kWavHeaderBytes])
{
    constexpr uint16_t kChannels = 2;
    constexpr uint16_t kBits = 16;
    constexpr uint16_t kBlockAlign = kChannels * (kBits / 8);
    constexpr uint32_t kByteRate = kOutputRate * kBlockAlign;
    const uint32_t riff_size = data_bytes > 0xFFFFFFFFu - 36u ? 0xFFFFFFFFu : data_bytes + 36u;
    std::memcpy(header + 0, "RIFF", 4);
    WriteLe32(header + 4, riff_size);
    std::memcpy(header + 8, "WAVE", 4);
    std::memcpy(header + 12, "fmt ", 4);
    WriteLe32(header + 16, 16);
    WriteLe16(header + 20, kFormatPcm);
    WriteLe16(header + 22, kChannels);
    WriteLe32(header + 24, kOutputRate);
    WriteLe32(header + 28, kByteRate);
    WriteLe16(header + 32, kBlockAlign);
    WriteLe16(header + 34, kBits);
    std::memcpy(header + 36, "data", 4);
    WriteLe32(header + 40, data_bytes);
}

void FailDump(const char* detail)
{
    if (!g_dump_error_reported)
    {
        g_dump_error_reported = true;
        std::fprintf(stderr, "[dsound] audio dump write failed (%s); turning dump off\n", detail);
    }
    if (g_dump_file != nullptr)
    {
        std::fclose(g_dump_file);
        g_dump_file = nullptr;
    }
}

void OpenDumpFile(const std::string& path)
{
    if (path.empty())
    {
        return;
    }
    if (g_dump_file != nullptr)
    {
        std::fclose(g_dump_file);
        g_dump_file = nullptr;
    }
    g_dump_error_reported = false;
    g_dump_file = std::fopen(path.c_str(), "wb");
    if (g_dump_file == nullptr)
    {
        std::fprintf(stderr, "[dsound] cannot open REXBOX_AUDIO_DUMP '%s': %s\n", path.c_str(), std::strerror(errno));
        g_dump_error_reported = true;
        return;
    }
    uint8_t header[kWavHeaderBytes];
    BuildWavHeader(0, header);
    if (std::fwrite(header, 1, kWavHeaderBytes, g_dump_file) != kWavHeaderBytes || std::fflush(g_dump_file) != 0)
    {
        FailDump(std::strerror(errno));
        return;
    }
    g_dump_bytes = 0;
    g_dump_last_header_bytes = 0;
    std::fprintf(stderr, "[dsound] audio dump: %s (%u Hz stereo 16-bit PCM)\n", path.c_str(), kOutputRate);
}

bool RewriteDumpHeader()
{
    if (g_dump_file == nullptr)
    {
        return false;
    }
    const uint32_t clamped_bytes =
        g_dump_bytes > 0xFFFFFFFFull - 36u ? 0xFFFFFFFFu - 36u : static_cast<uint32_t>(g_dump_bytes);
    uint8_t header[kWavHeaderBytes];
    BuildWavHeader(clamped_bytes, header);
    if (std::fseek(g_dump_file, 0, SEEK_SET) != 0 ||
        std::fwrite(header, 1, kWavHeaderBytes, g_dump_file) != kWavHeaderBytes ||
        std::fseek(g_dump_file, 0, SEEK_END) != 0 || std::fflush(g_dump_file) != 0)
    {
        FailDump(std::strerror(errno));
        return false;
    }
    g_dump_last_header_bytes = g_dump_bytes;
    return true;
}

void WriteDumpSamples(const float* samples, uint32_t frames)
{
    if (g_dump_file == nullptr || frames == 0)
    {
        return;
    }
    thread_local std::vector<uint8_t> pcm;
    const size_t count = static_cast<size_t>(frames) * 2;
    pcm.resize(count * 2);
    for (size_t i = 0; i < count; ++i)
    {
        const int32_t scaled = static_cast<int32_t>(std::lrint(samples[i] * 32767.0f));
        const auto s16 = static_cast<int16_t>(std::clamp(scaled, -32768, 32767));
        WriteLe16(pcm.data() + i * 2, static_cast<uint16_t>(s16));
    }
    if (std::fwrite(pcm.data(), 1, pcm.size(), g_dump_file) != pcm.size())
    {
        FailDump(std::strerror(errno));
        return;
    }
    g_dump_bytes += pcm.size();
    if (g_dump_bytes - g_dump_last_header_bytes >= kDumpHeaderRefreshBytes)
    {
        RewriteDumpHeader();
    }
}

void RenderOutput(float* samples, uint32_t frames, void* /*context*/)
{
    std::memset(samples, 0, sizeof(float) * 2 * frames);
    bool trace = false;
    {
        DsoundState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        trace = state.trace;
        if (trace)
        {
            uint32_t playing = 0;
            for (const auto& entry : state.buffers)
            {
                if (entry.second.playing)
                {
                    ++playing;
                }
            }
            state.max_playing_buffers = std::max(state.max_playing_buffers, playing);
        }
        MixLocked(samples, frames);
    }
    float peak = 0.0f;
    for (uint32_t i = 0; i < 2 * frames; ++i)
    {
        if (trace)
        {
            peak = std::max(peak, std::fabs(samples[i]));
        }
        samples[i] = std::clamp(samples[i], -1.0f, 1.0f);
    }
    if (trace)
    {
        DsoundState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        state.output_peak = std::max(state.output_peak, peak);
    }
    WriteDumpSamples(samples, frames);
}

}  // namespace

uint32_t TotalFrames(const Buffer& buffer)
{
    return FramesFromBytes(buffer, buffer.bytes);
}

uint32_t BytesFromFrames(const Buffer& buffer, uint32_t frames)
{
    if (IsAdpcm(buffer))
    {
        return (frames / kAdpcmSamplesPerBlock) * BlockAlign(buffer);
    }
    return frames * BlockAlign(buffer);
}

uint32_t FramesFromBytes(const Buffer& buffer, uint32_t bytes)
{
    if (IsAdpcm(buffer))
    {
        return (bytes / BlockAlign(buffer)) * kAdpcmSamplesPerBlock;
    }
    return bytes / BlockAlign(buffer);
}

void UpdateGains(Buffer& buffer)
{
    std::fill(std::begin(buffer.gain_left), std::end(buffer.gain_left), 0.0f);
    std::fill(std::begin(buffer.gain_right), std::end(buffer.gain_right), 0.0f);
    const uint32_t channels = std::clamp<uint32_t>(buffer.format.channels, 1, kMaxChannels);
    // The original stores the volume minus the headroom (design i040 section 2).
    const int64_t volume = static_cast<int64_t>(buffer.volume) - static_cast<int64_t>(buffer.headroom);
    const float master = VolumeGain(static_cast<int32_t>(std::max<int64_t>(volume, -10000)));
    if (!buffer.has_mix_bins)
    {
        if (channels == 1)
        {
            buffer.gain_left[0] = master * master;
            buffer.gain_right[0] = master * master;
        }
        else
        {
            for (uint32_t c = 0; c < channels; ++c)
            {
                AddBin(buffer, c, c, master);
            }
        }
    }
    else
    {
        for (size_t k = 0; k < buffer.mix_bins.size(); ++k)
        {
            const MixBinPair& pair = buffer.mix_bins[k];
            AddBin(buffer, static_cast<uint32_t>(k % channels), pair.bin, master * VolumeGain(pair.volume));
        }
    }
    for (uint32_t c = 0; c < channels; ++c)
    {
        buffer.gain_left[c] = std::sqrt(buffer.gain_left[c]);
        buffer.gain_right[c] = std::sqrt(buffer.gain_right[c]);
    }
    if (Is3dActive(buffer))
    {
        // A 3D voice: the mean of its speaker gains, attenuated and panned.
        const Spatial& spatial = buffer.spatial;
        for (uint32_t c = 0; c < channels; ++c)
        {
            const float base = 0.5f * (buffer.gain_left[c] + buffer.gain_right[c]) * spatial.attenuation;
            buffer.gain_left[c] = base * spatial.pan_left;
            buffer.gain_right[c] = base * spatial.pan_right;
        }
    }
}

void MixLocked(float* samples, uint32_t frames)
{
    for (auto& entry : State().buffers)
    {
        MixBuffer(entry.first, entry.second, samples, frames);
    }
    MixStreamsLocked(samples, frames);
}

bool WriteWavDumpForTest(const char* path, const float* samples, uint32_t frames)
{
    OpenDumpFile(path != nullptr ? path : "");
    if (g_dump_file == nullptr)
    {
        return false;
    }
    WriteDumpSamples(samples, frames);
    const bool ok = RewriteDumpHeader();
    std::fclose(g_dump_file);
    g_dump_file = nullptr;
    return ok;
}

void StartOutputLocked()
{
    DsoundState& state = State();
    if (state.output_started || state.output == DsoundOutput::kManual)
    {
        return;
    }
    state.output_started = true;
    OpenDumpFile(state.dump_path);
    platform::AudioOutputConfig config;
    config.sample_rate = kOutputRate;
    config.null_device = state.output == DsoundOutput::kNull;
    platform::AudioOutputInfo info;
    // The output thread takes the mutex this caller holds only once it has
    // started rendering, which waits for the caller to release it.
    if (!platform::StartAudioOutput(config, &RenderOutput, nullptr, &info))
    {
        std::fprintf(stderr, "[dsound] no audio output (%s): play cursors do not move\n", info.reason.c_str());
        return;
    }
    if (info.host_device)
    {
        std::fprintf(stderr, "[dsound] audio output: host device, %u Hz stereo\n", info.sample_rate);
    }
    else
    {
        std::fprintf(stderr, "[dsound] audio output: none (%s); cursors run in real time\n", info.reason.c_str());
    }
}

}  // namespace rexbox::hle::dsound
