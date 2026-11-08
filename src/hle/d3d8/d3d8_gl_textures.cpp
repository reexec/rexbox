#include "d3d8_gl_textures.h"

#include "rexbox/hle/d3d8/d3d8_display.h"

#include "d3d8_gl.h"
#include "d3d8_gl_targets.h"
#include "d3d8_globals.h"
#include "d3d8_internal.h"
#include "d3d8_render_state.h"
#include "d3d8_resource.h"
#include "d3d8_texture_format.h"
#include "d3d8_trace.h"

#include <algorithm>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace rexbox::hle::d3d8
{

namespace
{

constexpr uint32_t kHashSamples = 256;
// P8 variants kept per texture header; past this the least recently used goes.
constexpr size_t kMaxPaletteVariants = 16;

// D3DTEXTUREADDRESS and D3DTEXTUREFILTERTYPE (Xbox; inferred, PC numbering
// plus CLAMPTOEDGE 5).
constexpr uint32_t kAddressWrap = 1;
constexpr uint32_t kAddressMirror = 2;
constexpr uint32_t kAddressBorder = 4;
constexpr uint32_t kFilterNone = 0;
constexpr uint32_t kFilterPoint = 1;

struct CachedTexture
{
    BoundTexture bound;
    GLenum target = kGlTexture2D;
    uint32_t format_field = 0;
    uint32_t size_field = 0;
    uint32_t data = 0;
    uint32_t hash = 0;
    GuestAddress palette = 0;      // P8: the stage's palette at the upload
    uint32_t palette_data = 0;     // its Data and Common (the variant's key), and a hash of its entries
    uint32_t palette_common = 0;
    uint32_t palette_hash = 0;
    uint32_t checked_frame = 0xFFFFFFFF;
    uint32_t used_frame = 0;
    bool written = false;
    uint64_t gl_bytes = 0;  // RGBA8 levels uploaded (for the settings menu)
    bool storage = false;   // immutable storage allocated via glTexStorage2D
    GLenum storage_format = 0;
    uint32_t storage_width = 0;
    uint32_t storage_height = 0;
    uint32_t storage_levels = 0;
};

// A texture header's GL copies: one, or for P8 one per palette a title draws
// it with (the same texture in several colourings).
struct TextureEntry
{
    std::vector<CachedTexture> variants;
};

std::unordered_map<GuestAddress, TextureEntry> g_textures;

// Whether DXT goes up compressed: S3TC on the host, unless REXBOX_DXT=cpu
// asks for the CPU decode (for comparison). Reported once.
bool DxtCompressedPath()
{
    static const bool compressed = [] {
        const char* setting = std::getenv("REXBOX_DXT");
        const bool forced_cpu = setting != nullptr && std::strcmp(setting, "cpu") == 0;
        const bool result = GlHasS3tc() && !forced_cpu;
        std::fprintf(stderr, "[d3d8] DXT textures: %s\n",
                     result       ? "uploaded compressed (S3TC)"
                     : forced_cpu ? "decoded on the CPU (REXBOX_DXT=cpu)"
                                  : "decoded on the CPU (no S3TC on this host)");
        return result;
    }();
    return compressed;
}

GLenum DxtInternalFormat(uint32_t format)
{
    switch (format)
    {
    case fmt::kDxt1:
        return kGlCompressedRgbaS3tcDxt1;  // index 3 transparent black when c0 <= c1, as on the Xbox
    case fmt::kDxt3:
        return kGlCompressedRgbaS3tcDxt3;
    default:
        return kGlCompressedRgbaS3tcDxt5;
    }
}

// One sampler object per combination of address modes and filters, so a
// draw binds one instead of changing a shared sampler's parameters.
std::map<std::pair<uint64_t, uint32_t>, GLuint> g_samplers;  // (wraps and filters, anisotropy)

// What each texture unit holds, valid while `epoch` is the GL state epoch.
struct UnitBinding
{
    uint32_t epoch = 0;
    GLenum target = 0;
    GLuint texture = 0;
    GLuint sampler = 0;
};
UnitBinding g_units[globals::kTextureStages];

// After an upload binds on unit 0, or a delete unbinds a texture everywhere.
void ForgetUnitBindings()
{
    for (UnitBinding& unit : g_units)
    {
        unit = {};
    }
}

// A cheap fingerprint of level 0: evenly spaced DWORDs.
uint32_t SampleHash(uint32_t data, uint32_t bytes)
{
    runtime::GuestMemory& memory = Memory();
    uint32_t hash = 2166136261u ^ bytes;
    const uint32_t words = bytes / 4;
    if (words == 0 || !memory.IsValid(data, bytes))
    {
        return hash;
    }
    const uint32_t step = std::max<uint32_t>(1, words / kHashSamples);
    for (uint32_t i = 0; i < words; i += step)
    {
        hash = (hash ^ memory.Read32(data + i * 4)) * 16777619u;
    }
    return hash;
}

struct UploadTiming
{
    bool storage_path = false;
    std::chrono::steady_clock::duration decode_time{};
    std::chrono::steady_clock::duration storage_time{};
    std::chrono::steady_clock::duration data_time{};
    std::chrono::steady_clock::duration gl_time{};
};

bool Upload(GuestAddress header, CachedTexture* cached, UploadTiming* timing = nullptr)
{
    GlFunctions& gl = Gl();
    runtime::GuestMemory& memory = Memory();
    const uint32_t format_field = cached->format_field;
    const uint32_t format = (format_field >> header::kFormatFormatShift) & 0xFF;
    const bool linear = cached->size_field != 0;
    const bool cube = (format_field & header::kFormatCubeMap) != 0;
    const uint32_t dimension = (format_field >> header::kFormatDimensionShift) & 0xF;
    TextureFormatInfo info;
    const bool paletted = DescribeTextureFormat(format, &info) && info.palette;
    std::vector<uint32_t> palette;
    if (paletted && cached->palette != 0)
    {
        const uint32_t entries = 256u >> (cached->palette_common >> header::kPaletteSizeShift);
        const GuestAddress entries_address = cached->palette_data & kPhysicalMask;
        if (memory.IsValid(entries_address, entries * 4))
        {
            palette.resize(entries);
            for (uint32_t i = 0; i < entries; ++i)
            {
                palette[i] = memory.Read32(entries_address + i * 4);
            }
        }
    }
    if (paletted && palette.empty())
    {
        ReportOnce(Format("texpal %08X", header),
                   Format("P8 texture %08X has no palette on its stage; sampled as white", header));
        return false;
    }
    if (!paletted && !DescribeTextureFormat(format, &info))
    {
        ReportOnce(Format("texfmt %02X", format), Format("texture format %02X is not decoded yet (header %08X); "
                                                          "sampled as white",
                                                          format, header));
        return false;
    }
    if (cube)
    {
        ReportOnce("cube", Format("cube texture %08X: cube maps are not uploaded yet; sampled as white", header));
        return false;
    }
    const uint32_t levels = linear ? 1 : std::max<uint32_t>(1, (format_field >> header::kFormatMipLevelsShift) & 0xF);
    const GLenum target = dimension == 3 ? kGlTexture3D : kGlTexture2D;
    const LevelLayout layout0 = HeaderLevelLayout(header, 0);
    const uint32_t log2_width = (format_field >> header::kFormatLog2WidthShift) & 0xF;
    const uint32_t log2_height = (format_field >> header::kFormatLog2HeightShift) & 0xF;
    const uint32_t width0 = linear ? layout0.width : std::max<uint32_t>(1, 1u << log2_width);
    const uint32_t height0 = linear ? layout0.height : std::max<uint32_t>(1, 1u << log2_height);

    // DXT goes up as it is (the GPU decodes it, as the NV2A did) when the host
    // has S3TC; volume DXT and hosts without it decode on the CPU
    // (docs/design/20261017-i088-dxt-compressed.md).
    const bool compressed = info.layout == PixelLayout::kCompressed && target == kGlTexture2D && DxtCompressedPath();
    const GLenum internal_format = compressed ? DxtInternalFormat(format) : static_cast<GLenum>(kGlRgba8);

    uint32_t valid_levels = 0;
    for (uint32_t level = 0; level < levels; ++level)
    {
        const LevelLayout layout = HeaderLevelLayout(header, level);
        const uint32_t width = linear ? layout.width : std::max<uint32_t>(1, (1u << log2_width) >> level);
        const uint32_t height = linear ? layout.height : std::max<uint32_t>(1, (1u << log2_height) >> level);
        const uint32_t depth = layout.depth;
        const uint32_t address = (cached->data + TextureLevelOffset(header, level)) & kPhysicalMask;
        const uint32_t bytes = LevelBytes(format, std::max(width, info.layout == PixelLayout::kCompressed ? 4u : 1u),
                                          std::max(height, info.layout == PixelLayout::kCompressed ? 4u : 1u), depth,
                                          linear ? layout.pitch : 0);
        if (!memory.IsValid(address, bytes))
        {
            break;
        }
        ++valid_levels;
    }

    const bool has_storage = gl.TexStorage2D != nullptr &&
                             (compressed ? gl.CompressedTexSubImage2D != nullptr : gl.TexSubImage2D != nullptr);
    const bool use_storage = has_storage && target == kGlTexture2D && valid_levels == levels;
    if (timing != nullptr)
    {
        timing->storage_path = use_storage;
    }

    const bool measure = timing != nullptr;
    auto gl_start = measure ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};

    if (use_storage)
    {
        const bool storage_matches = cached->storage && cached->bound.texture != 0 && cached->target == target &&
                                     cached->storage_format == internal_format && cached->storage_width == width0 &&
                                     cached->storage_height == height0 && cached->storage_levels == levels;
        if (!storage_matches)
        {
            if (cached->bound.texture != 0)
            {
                gl.DeleteTextures(1, &cached->bound.texture);
                cached->bound.texture = 0;
            }
            gl.GenTextures(1, &cached->bound.texture);
            cached->target = target;
            ForgetUnitBindings();
            gl.ActiveTexture(kGlTexture0);
            gl.BindTexture(target, cached->bound.texture);
            gl.TexStorage2D(target, static_cast<GLsizei>(levels), internal_format,
                            static_cast<GLsizei>(width0), static_cast<GLsizei>(height0));
            cached->storage = true;
            cached->storage_format = internal_format;
            cached->storage_width = width0;
            cached->storage_height = height0;
            cached->storage_levels = levels;
        }
        else
        {
            cached->target = target;
            ForgetUnitBindings();
            gl.ActiveTexture(kGlTexture0);
            gl.BindTexture(target, cached->bound.texture);
        }
        gl.PixelStorei(kGlUnpackAlignment, 1);
        if (measure)
        {
            timing->storage_time += std::chrono::steady_clock::now() - gl_start;
        }
    }
    else
    {
        if (cached->bound.texture == 0 || cached->target != target || cached->storage)
        {
            if (cached->bound.texture != 0)
            {
                gl.DeleteTextures(1, &cached->bound.texture);
                cached->bound.texture = 0;
            }
            gl.GenTextures(1, &cached->bound.texture);
            cached->storage = false;
        }
        cached->target = target;
        ForgetUnitBindings();
        gl.ActiveTexture(kGlTexture0);
        gl.BindTexture(target, cached->bound.texture);
        gl.PixelStorei(kGlUnpackAlignment, 1);
        if (measure)
        {
            timing->gl_time += std::chrono::steady_clock::now() - gl_start;
        }
    }

    std::vector<uint8_t> rgba;
    uint32_t uploaded = 0;
    cached->gl_bytes = 0;
    for (uint32_t level = 0; level < levels; ++level)
    {
        const LevelLayout layout = HeaderLevelLayout(header, level);
        const uint32_t width = linear ? layout.width : std::max<uint32_t>(1, (1u << log2_width) >> level);
        const uint32_t height = linear ? layout.height : std::max<uint32_t>(1, (1u << log2_height) >> level);
        const uint32_t depth = layout.depth;
        const uint32_t address = (cached->data + TextureLevelOffset(header, level)) & kPhysicalMask;
        const uint32_t bytes = LevelBytes(format, std::max(width, info.layout == PixelLayout::kCompressed ? 4u : 1u),
                                          std::max(height, info.layout == PixelLayout::kCompressed ? 4u : 1u), depth,
                                          linear ? layout.pitch : 0);
        if (!memory.IsValid(address, bytes))
        {
            ReportOnce(Format("texmem %08X", header),
                       Format("texture %08X level %u data %08X+%X is outside guest memory", header, level, address,
                              bytes));
            break;
        }
        if (compressed)
        {
            if (measure)
            {
                gl_start = std::chrono::steady_clock::now();
            }
            if (use_storage)
            {
                gl.CompressedTexSubImage2D(target, static_cast<GLint>(level), 0, 0,
                                           static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                                           internal_format, static_cast<GLsizei>(bytes),
                                           memory.Translate(address));
                if (measure)
                {
                    timing->data_time += std::chrono::steady_clock::now() - gl_start;
                }
            }
            else
            {
                gl.CompressedTexImage2D(target, static_cast<GLint>(level), internal_format,
                                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                                        static_cast<GLsizei>(bytes), memory.Translate(address));
                if (measure)
                {
                    timing->gl_time += std::chrono::steady_clock::now() - gl_start;
                }
            }
            if (level == 0)
            {
                cached->bound.width = width;
                cached->bound.height = height;
            }
            cached->gl_bytes += bytes;
            ++uploaded;
            continue;
        }
        // Compressed levels smaller than a block decode from the padded block.
        const uint32_t decode_width = info.layout == PixelLayout::kCompressed ? std::max(width, 4u) : width;
        const uint32_t decode_height = info.layout == PixelLayout::kCompressed ? std::max(height, 4u) : height;
        auto decode_start = measure ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        if (paletted)
        {
            DecodePalettedLevel(memory.Translate(address), width, height, depth, palette.data(),
                                static_cast<uint32_t>(palette.size()), &rgba);
        }
        else if (!DecodeTextureLevel(format, memory.Translate(address), decode_width, decode_height, depth,
                                     linear ? layout.pitch : 0, &rgba))
        {
            break;
        }
        if (decode_width != width || decode_height != height)
        {
            // Keep the top-left texels of the padded block.
            std::vector<uint8_t> cropped(static_cast<size_t>(width) * height * 4);
            for (uint32_t y = 0; y < height; ++y)
            {
                std::copy_n(rgba.data() + static_cast<size_t>(y) * decode_width * 4, width * 4,
                            cropped.data() + static_cast<size_t>(y) * width * 4);
            }
            rgba.swap(cropped);
        }
        if (measure)
        {
            timing->decode_time += std::chrono::steady_clock::now() - decode_start;
            gl_start = std::chrono::steady_clock::now();
        }
        if (use_storage)
        {
            gl.TexSubImage2D(target, static_cast<GLint>(level), 0, 0, static_cast<GLsizei>(width),
                             static_cast<GLsizei>(height), kGlRgba, kGlUnsignedByte, rgba.data());
            if (measure)
            {
                timing->data_time += std::chrono::steady_clock::now() - gl_start;
            }
        }
        else
        {
            if (target == kGlTexture3D)
            {
                gl.TexImage3D(target, static_cast<GLint>(level), static_cast<GLint>(kGlRgba8),
                              static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                              static_cast<GLsizei>(depth), 0, kGlRgba, kGlUnsignedByte, rgba.data());
            }
            else
            {
                gl.TexImage2D(target, static_cast<GLint>(level), static_cast<GLint>(kGlRgba8),
                              static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, kGlRgba,
                              kGlUnsignedByte, rgba.data());
            }
            if (measure)
            {
                timing->gl_time += std::chrono::steady_clock::now() - gl_start;
            }
        }
        if (level == 0)
        {
            cached->bound.width = width;
            cached->bound.height = height;
        }
        cached->gl_bytes += static_cast<uint64_t>(width) * height * std::max(depth, 1u) * 4;
        ++uploaded;
    }
    if (uploaded == 0)
    {
        return false;
    }
    if (measure)
    {
        gl_start = std::chrono::steady_clock::now();
    }
    gl.TexParameteri(target, kGlTextureBaseLevel, 0);
    gl.TexParameteri(target, kGlTextureMaxLevel, static_cast<GLint>(uploaded - 1));
    if (measure)
    {
        if (use_storage)
        {
            timing->data_time += std::chrono::steady_clock::now() - gl_start;
        }
        else
        {
            timing->gl_time += std::chrono::steady_clock::now() - gl_start;
        }
    }
    cached->bound.levels = uploaded;
    cached->bound.format = format;
    cached->bound.kind = target == kGlTexture3D ? SamplerKind::k3D : SamplerKind::k2D;
    cached->bound.unnormalized = linear;
    if (D3d8TraceEnabled())
    {
        TraceOnce(Format("upload %02X %u %u", format, cached->bound.width, cached->bound.height),
                  Format("texture upload: format %02X %ux%u, %u level(s)%s", format, cached->bound.width,
                         cached->bound.height, uploaded, linear ? ", linear (texel coordinates)" : ""));
    }
    return true;
}

void DeleteVariant(CachedTexture& variant)
{
    if (variant.bound.texture != 0)
    {
        Gl().DeleteTextures(1, &variant.bound.texture);
        ForgetUnitBindings();
        variant.bound.texture = 0;
        variant.storage = false;
    }
}

// The variant for this palette (Data and Common), made on first use; a
// texture that is not P8 has exactly one.
CachedTexture& SelectVariant(TextureEntry& entry, bool paletted, uint32_t palette_data, uint32_t palette_common,
                             uint32_t frame)
{
    std::vector<CachedTexture>& variants = entry.variants;
    if (!paletted)
    {
        while (variants.size() > 1)
        {
            DeleteVariant(variants.back());
            variants.pop_back();
        }
        if (variants.empty())
        {
            variants.emplace_back();
        }
        variants[0].used_frame = frame;
        return variants[0];
    }
    for (CachedTexture& variant : variants)
    {
        if (variant.palette_data == palette_data && variant.palette_common == palette_common)
        {
            variant.used_frame = frame;
            return variant;
        }
    }
    CachedTexture* slot = nullptr;
    if (variants.size() < kMaxPaletteVariants)
    {
        slot = &variants.emplace_back();
    }
    else
    {
        slot = &*std::min_element(variants.begin(), variants.end(), [](const CachedTexture& a, const CachedTexture& b) {
            return a.used_frame < b.used_frame;
        });
        DeleteVariant(*slot);
        *slot = CachedTexture{};
    }
    slot->palette_data = palette_data;
    slot->palette_common = palette_common;
    slot->used_frame = frame;
    return *slot;
}

GLint AddressMode(uint32_t value)
{
    switch (value)
    {
    case kAddressWrap:
        return static_cast<GLint>(kGlRepeat);
    case kAddressMirror:
        return static_cast<GLint>(kGlMirroredRepeat);
    case kAddressBorder:
        ReportOnce("border", "texture address mode BORDER drawn as CLAMP (GLES has no border color)");
        return static_cast<GLint>(kGlClampToEdge);
    default:
        return static_cast<GLint>(kGlClampToEdge);
    }
}

}  // namespace

BoundTexture ResolveTexture(GuestAddress header, GuestAddress palette)
{
    if (header == 0)
    {
        return {};
    }
    runtime::GuestMemory& memory = Memory();
    const uint32_t format_field = memory.Read32(header + header::kFormat);
    const uint32_t size_field = memory.Read32(header + header::kSize);
    const uint32_t data = memory.Read32(header + header::kData);
    // Pixels a render target holds live on the GPU, not in guest memory.
    GuestAddress surface = 0;
    BoundTexture target;
    target.texture = SampleRenderTarget(data, &surface, &target.width, &target.height);
    if (target.texture != 0)
    {
        if (FirstReport("rt sample", header))
        {
            ReportOnce(Format("rt sample %08X", header),
                       Format("texture %08X samples render target %08X at %08X (%ux%u)", header, surface,
                              data & kPhysicalMask, target.width, target.height));
        }
        target.kind = SamplerKind::k2D;
        target.unnormalized = size_field != 0;
        return target;
    }
    // A P8 texture also depends on its stage's palette: each palette has its
    // own variant, so switching palettes between draws does not decode again.
    const bool paletted = ((format_field >> header::kFormatFormatShift) & 0xFF) == fmt::kP8;
    const uint32_t palette_data = paletted && palette != 0 ? memory.Read32(palette + header::kData) : 0;
    const uint32_t palette_common = paletted && palette != 0 ? memory.Read32(palette + header::kCommon) : 0;
    const uint32_t palette_bytes = (256u >> (palette_common >> header::kPaletteSizeShift)) * 4;
    const uint32_t frame = State().frame_count;
    CachedTexture& cached = SelectVariant(g_textures[header], paletted, palette_data, palette_common, frame);
    const bool is_new = cached.bound.texture == 0;
    const bool is_written = cached.written;
    const bool is_fields = format_field != cached.format_field || size_field != cached.size_field ||
                          data != cached.data;
    bool upload = is_new || is_written || is_fields;
    // Writes without a lock: compare a sampled hash once per frame.
    const LevelLayout layout0 = HeaderLevelLayout(header, 0);
    bool is_hash = false;
    if (!upload && cached.checked_frame != frame)
    {
        cached.checked_frame = frame;
        is_hash = SampleHash(data & kPhysicalMask, layout0.size) != cached.hash ||
                 (paletted && palette != 0 &&
                  SampleHash(palette_data & kPhysicalMask, palette_bytes) != cached.palette_hash);
        upload = is_hash;
    }
    if (upload)
    {
        profile::TextureUploadReason reason = profile::TextureUploadReason::kNew;
        if (is_new)
        {
            reason = profile::TextureUploadReason::kNew;
        }
        else if (is_written)
        {
            reason = profile::TextureUploadReason::kReuploadWritten;
        }
        else if (is_fields)
        {
            reason = profile::TextureUploadReason::kReuploadFields;
        }
        else
        {
            reason = profile::TextureUploadReason::kReuploadHash;
        }
        const uint32_t format = (format_field >> header::kFormatFormatShift) & 0xFF;
        const bool linear = size_field != 0;
        const uint32_t log2_width = (format_field >> header::kFormatLog2WidthShift) & 0xF;
        const uint32_t log2_height = (format_field >> header::kFormatLog2HeightShift) & 0xF;
        const uint32_t width0 = linear ? layout0.width : std::max<uint32_t>(1, (1u << log2_width));
        const uint32_t height0 = linear ? layout0.height : std::max<uint32_t>(1, (1u << log2_height));
        const bool same_size =
            !is_new && width0 == cached.bound.width && height0 == cached.bound.height && format == cached.bound.format;

        cached.format_field = format_field;
        cached.size_field = size_field;
        cached.data = data;
        cached.written = false;
        cached.checked_frame = frame;
        cached.hash = SampleHash(data & kPhysicalMask, layout0.size);
        cached.palette = paletted ? palette : 0;
        cached.palette_data = palette_data;
        cached.palette_common = palette_common;
        cached.palette_hash = paletted && palette != 0 ? SampleHash(palette_data & kPhysicalMask, palette_bytes) : 0;
        UploadTiming timing;
        const bool detail = profile::SlowFrameDetailEnabled();
        const auto started = std::chrono::steady_clock::now();
        const bool ok = Upload(header, &cached, detail ? &timing : nullptr);
        if (!ok)
        {
            cached.bound.kind = SamplerKind::kNone;
        }
        profile::CountTextureUpload(cached.gl_bytes, std::chrono::steady_clock::now() - started);
        if (detail && ok)
        {
            profile::NoteTextureUpload(header, format, width0, height0, cached.bound.levels, cached.gl_bytes, reason,
                                       same_size, timing.decode_time, timing.gl_time, timing.storage_time,
                                       timing.data_time, timing.storage_path);
        }
    }
    return cached.bound.kind == SamplerKind::kNone ? BoundTexture{} : cached.bound;
}

// The sampler object for these parameters, created on first use.
GLuint Sampler(GLint wrap_s, GLint wrap_t, GLint wrap_r, GLenum mag_filter, GLenum min_filter, uint32_t anisotropy)
{
    // GL enum values fit in 16 bits; the anisotropy is the key's second half.
    const std::pair<uint64_t, uint32_t> key{(static_cast<uint64_t>(wrap_s & 0xFFFF) << 48) |
                                                (static_cast<uint64_t>(wrap_t & 0xFFFF) << 32) |
                                                (static_cast<uint64_t>(wrap_r & 0xFFFF) << 16) |
                                                (static_cast<uint64_t>(mag_filter == kGlNearest ? 1 : 0) << 15) |
                                                (min_filter & 0x7FFF),
                                            anisotropy};
    const auto it = g_samplers.find(key);
    if (it != g_samplers.end())
    {
        return it->second;
    }
    GlFunctions& gl = Gl();
    GLuint sampler = 0;
    gl.GenSamplers(1, &sampler);
    gl.SamplerParameteri(sampler, kGlTextureWrapS, wrap_s);
    gl.SamplerParameteri(sampler, kGlTextureWrapT, wrap_t);
    gl.SamplerParameteri(sampler, kGlTextureWrapR, wrap_r);
    gl.SamplerParameteri(sampler, kGlTextureMagFilter, static_cast<GLint>(mag_filter));
    gl.SamplerParameteri(sampler, kGlTextureMinFilter, static_cast<GLint>(min_filter));
    if (anisotropy > 1 && gl.SamplerParameterf != nullptr)
    {
        gl.SamplerParameterf(sampler, kGlTextureMaxAnisotropy, static_cast<GLfloat>(anisotropy));
    }
    g_samplers.emplace(key, sampler);
    return sampler;
}

void BindStageTexture(uint32_t stage, const BoundTexture& texture)
{
    GlFunctions& gl = Gl();
    const bool mag_point = TextureStateValue(stage, tss::kMagFilter) == kFilterPoint;
    const bool min_point = TextureStateValue(stage, tss::kMinFilter) == kFilterPoint;
    const uint32_t mip = texture.levels > 1 ? TextureStateValue(stage, tss::kMipFilter) : kFilterNone;
    GLenum min_filter = min_point ? kGlNearest : kGlLinear;
    if (mip == kFilterPoint)
    {
        min_filter = min_point ? kGlNearestMipmapNearest : kGlLinearMipmapNearest;
    }
    else if (mip != kFilterNone)
    {
        min_filter = min_point ? kGlNearestMipmapLinear : kGlLinearMipmapLinear;
    }
    // The user's anisotropic filtering, on linear minification only (point
    // filtering is the title's choice; docs/design/20261017-i089-anti-aliasing.md).
    const uint32_t anisotropy = min_point ? 1u : Anisotropy();
    const GLint wrap_s = AddressMode(TextureStateValue(stage, tss::kAddressU));
    const GLint wrap_t = AddressMode(TextureStateValue(stage, tss::kAddressV));
    const GLint wrap_r = AddressMode(TextureStateValue(stage, tss::kAddressW));
    const GLenum mag_filter = mag_point ? kGlNearest : kGlLinear;
    // The stage's previous sampler when its parameters are unchanged, so most
    // draws skip the map (issue #23); sampler objects are never deleted.
    struct LastSampler
    {
        GLint wrap[3] = {};
        GLenum mag = 0;
        GLenum min = 0;
        uint32_t anisotropy = 0;
        GLuint sampler = 0;
    };
    static LastSampler last[globals::kTextureStages];
    LastSampler& previous = last[stage];
    if (previous.sampler == 0 || previous.wrap[0] != wrap_s || previous.wrap[1] != wrap_t ||
        previous.wrap[2] != wrap_r || previous.mag != mag_filter || previous.min != min_filter ||
        previous.anisotropy != anisotropy)
    {
        previous = {{wrap_s, wrap_t, wrap_r}, mag_filter, min_filter, anisotropy,
                    Sampler(wrap_s, wrap_t, wrap_r, mag_filter, min_filter, anisotropy)};
    }
    const GLuint sampler = previous.sampler;
    const GLenum target = texture.kind == SamplerKind::k3D ? kGlTexture3D : kGlTexture2D;

    UnitBinding& unit = g_units[stage];
    const uint32_t epoch = GlStateEpoch();
    if (unit.epoch != epoch)
    {
        unit = {};
        unit.epoch = epoch;
    }
    if (unit.target != target || unit.texture != texture.texture)
    {
        gl.ActiveTexture(kGlTexture0 + stage);
        gl.BindTexture(target, texture.texture);
        unit.target = target;
        unit.texture = texture.texture;
    }
    if (unit.sampler != sampler)
    {
        gl.BindSampler(stage, sampler);
        unit.sampler = sampler;
    }
}

TextureCacheUse TextureCacheBytes()
{
    TextureCacheUse use;
    for (const auto& [header, entry] : g_textures)
    {
        for (const CachedTexture& cached : entry.variants)
        {
            if (cached.bound.texture != 0)
            {
                ++use.count;
                use.bytes += cached.gl_bytes;
            }
        }
    }
    return use;
}

void MarkTextureWritten(GuestAddress header)
{
    const auto it = g_textures.find(header);
    if (it != g_textures.end())
    {
        for (CachedTexture& cached : it->second.variants)
        {
            cached.written = true;
        }
    }
}

void ForgetTexture(GuestAddress header)
{
    const auto it = g_textures.find(header);
    if (it == g_textures.end())
    {
        return;
    }
    for (CachedTexture& cached : it->second.variants)
    {
        DeleteVariant(cached);
    }
    g_textures.erase(it);
}

}  // namespace rexbox::hle::d3d8
