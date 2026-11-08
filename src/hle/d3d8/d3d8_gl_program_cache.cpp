#include "d3d8_gl_program_cache.h"

#include "d3d8_gl.h"
#include "d3d8_profile.h"
#include "d3d8_trace.h"

#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rexbox::hle::d3d8
{

namespace
{

// File layout: magic, layout version, binary format, then four
// length-prefixed blocks: driver identity, vertex source, fragment source,
// program binary. All integers little-endian uint32.
constexpr char kMagic[4] = {'R', 'X', 'P', 'B'};
constexpr uint32_t kLayoutVersion = 1;

struct CacheState
{
    bool checked = false;
    bool enabled = false;
    std::filesystem::path directory;
    std::string driver;  // vendor, renderer and version
};

CacheState& Cache()
{
    static CacheState state;
    if (state.checked)
    {
        return state;
    }
    state.checked = true;
    const char* setting = std::getenv("REXBOX_SHADER_CACHE");
    if (setting != nullptr && (std::strcmp(setting, "0") == 0 || std::strcmp(setting, "off") == 0))
    {
        return state;
    }
    GlFunctions& gl = Gl();
    if (gl.GetProgramBinary == nullptr || gl.ProgramBinary == nullptr || gl.ProgramParameteri == nullptr)
    {
        return state;
    }
    GLint formats = 0;
    gl.GetIntegerv(kGlNumProgramBinaryFormats, &formats);
    if (formats <= 0)
    {
        return state;
    }
    for (const GLenum name : {kGlVendor, kGlRenderer, kGlVersion})
    {
        const GLubyte* text = gl.GetString(name);
        state.driver += text != nullptr ? reinterpret_cast<const char*>(text) : "?";
        state.driver += '\n';
    }
    state.directory = setting != nullptr && setting[0] != '\0' ? std::filesystem::path(setting)
                                                               : std::filesystem::path("cache") / "shaders";
    std::error_code error;
    std::filesystem::create_directories(state.directory, error);
    state.enabled = !error;
    if (!state.enabled)
    {
        ReportOnce("shader cache", Format("shader cache directory %s cannot be created; programs are not cached",
                                          state.directory.string().c_str()));
    }
    return state;
}

uint64_t Hash(const std::string& a, const std::string& b, const std::string& c)
{
    uint64_t hash = 1469598103934665603ull;
    for (const std::string* text : {&a, &b, &c})
    {
        for (const unsigned char ch : *text)
        {
            hash = (hash ^ ch) * 1099511628211ull;
        }
        hash = (hash ^ 0xFF) * 1099511628211ull;
    }
    return hash;
}

std::filesystem::path EntryPath(const std::string& vertex_source, const std::string& fragment_source)
{
    CacheState& cache = Cache();
    char name[32];
    std::snprintf(name, sizeof(name), "%016llX.bin",
                  static_cast<unsigned long long>(Hash(cache.driver, vertex_source, fragment_source)));
    return cache.directory / name;
}

void PutU32(std::vector<char>* out, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        out->push_back(static_cast<char>((value >> (8 * i)) & 0xFF));
    }
}

void PutBlock(std::vector<char>* out, const char* data, size_t size)
{
    PutU32(out, static_cast<uint32_t>(size));
    out->insert(out->end(), data, data + size);
}

// Reads the file's blocks; false when it is not a complete entry.
struct Reader
{
    const std::vector<char>& data;
    size_t at = 0;

    bool U32(uint32_t* value)
    {
        if (data.size() - at < 4)
        {
            return false;
        }
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i)
        {
            v |= static_cast<uint32_t>(static_cast<unsigned char>(data[at + i])) << (8 * i);
        }
        at += 4;
        *value = v;
        return true;
    }
    bool Block(const char** begin, size_t* size)
    {
        uint32_t length = 0;
        if (!U32(&length) || data.size() - at < length)
        {
            return false;
        }
        *begin = data.data() + at;
        *size = length;
        at += length;
        return true;
    }
    bool Matches(const std::string& expected)
    {
        const char* begin = nullptr;
        size_t size = 0;
        return Block(&begin, &size) && size == expected.size() && std::memcmp(begin, expected.data(), size) == 0;
    }
};

}  // namespace

bool ProgramCacheEnabled()
{
    return Cache().enabled;
}

void MarkProgramRetrievable(uint32_t program)
{
    if (ProgramCacheEnabled())
    {
        Gl().ProgramParameteri(program, kGlProgramBinaryRetrievableHint, 1);
    }
}

namespace
{

// A cache file's contents, in one read.
bool ReadFile(const std::filesystem::path& path, std::vector<char>* data)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size < 12)
    {
        return false;
    }
    data->resize(static_cast<size_t>(size));
    file.seekg(0);
    return static_cast<bool>(file.read(data->data(), size));
}

// An entry's parts (pointing into `data`); false when it is not a complete
// entry of this layout and driver.
struct Entry
{
    uint32_t format = 0;
    const char* vertex = nullptr;
    size_t vertex_size = 0;
    const char* fragment = nullptr;
    size_t fragment_size = 0;
    const char* binary = nullptr;
    size_t binary_size = 0;
};

bool ParseEntry(const std::vector<char>& data, Entry* entry)
{
    if (data.size() < 12 || std::memcmp(data.data(), kMagic, 4) != 0)
    {
        return false;
    }
    Reader reader{data, 4};
    uint32_t layout = 0;
    return reader.U32(&layout) && layout == kLayoutVersion && reader.U32(&entry->format) &&
           reader.Matches(Cache().driver) && reader.Block(&entry->vertex, &entry->vertex_size) &&
           reader.Block(&entry->fragment, &entry->fragment_size) && reader.Block(&entry->binary, &entry->binary_size);
}

// The program from a stored binary, or 0 when the driver no longer takes it
// (an update, say).
GLuint MakeProgram(uint32_t format, const char* binary, size_t size)
{
    GlFunctions& gl = Gl();
    const GLuint program = gl.CreateProgram();
    gl.ProgramBinary(program, format, binary, static_cast<GLsizei>(size));
    GLint linked = 0;
    gl.GetProgramiv(program, kGlLinkStatus, &linked);
    if (linked == 0)
    {
        gl.DeleteProgram(program);
        return 0;
    }
    return program;
}

// A second hash of a program's sources (another seed than the entry name's),
// so a warmed program is handed out only for the sources it was made from.
uint64_t SourceCheck(const std::string& vertex, const std::string& fragment)
{
    return Hash("source check", vertex, fragment);
}

// An entry read and checked off the GL thread, waiting to be made.
struct ReadyEntry
{
    std::string name;  // the entry's file name
    uint32_t format = 0;
    std::vector<char> binary;
    uint64_t check = 0;
};

// The reader thread lists the cache directory most recently used first,
// reads and checks up to kReadAhead entries ahead of the GL thread, and marks
// the entries the GL thread used, so file reads and file times never stall
// the presenting thread. It lives as long as the process.
struct WarmReader
{
    static constexpr size_t kReadAhead = 16;
    std::mutex mutex;
    std::condition_variable wake;
    std::deque<ReadyEntry> read;
    std::vector<std::filesystem::path> touch;
    bool done = false;  // every listed entry read
};

WarmReader* g_reader = nullptr;  // made on the first warming, never freed

size_t WarmLimit()
{
    static const size_t limit = [] {
        const char* value = std::getenv("REXBOX_SHADER_WARM");
        return value != nullptr ? static_cast<size_t>(std::strtoul(value, nullptr, 10)) : size_t{1024};
    }();
    return limit;
}

// Marks an entry as just used: warming takes the most recent first.
void Touch(const std::filesystem::path& path)
{
    std::error_code ignored;
    std::filesystem::last_write_time(path, std::filesystem::file_time_type::clock::now(), ignored);
}

std::vector<std::filesystem::path> ListEntries(const std::filesystem::path& directory)
{
    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> entries;
    std::error_code error;
    for (const auto& item : std::filesystem::directory_iterator(directory, error))
    {
        if (item.path().extension() == ".bin")
        {
            std::error_code time_error;
            entries.emplace_back(item.last_write_time(time_error), item.path());
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    entries.resize(std::min(entries.size(), WarmLimit()));
    std::vector<std::filesystem::path> paths;
    for (auto& entry : entries)
    {
        paths.push_back(std::move(entry.second));
    }
    return paths;
}

void RunReader(WarmReader* reader, std::filesystem::path directory)
{
    const std::vector<std::filesystem::path> paths = ListEntries(directory);
    std::vector<char> data;
    size_t next = 0;
    std::unique_lock<std::mutex> lock(reader->mutex);
    for (;;)
    {
        reader->wake.wait(lock, [&] {
            return !reader->touch.empty() || (next < paths.size() && reader->read.size() < WarmReader::kReadAhead);
        });
        std::vector<std::filesystem::path> touch;
        touch.swap(reader->touch);
        const bool read_one = next < paths.size() && reader->read.size() < WarmReader::kReadAhead;
        lock.unlock();
        for (const std::filesystem::path& path : touch)
        {
            Touch(path);
        }
        ReadyEntry ready;
        bool have = false;
        if (read_one)
        {
            const std::filesystem::path& path = paths[next];
            Entry entry;
            if (ReadFile(path, &data) && ParseEntry(data, &entry))
            {
                ready.name = path.filename().string();
                ready.format = entry.format;
                ready.binary.assign(entry.binary, entry.binary + entry.binary_size);
                ready.check = SourceCheck(std::string(entry.vertex, entry.vertex_size),
                                          std::string(entry.fragment, entry.fragment_size));
                have = true;
            }
        }
        lock.lock();
        if (read_one)
        {
            ++next;
            reader->done = next == paths.size();
        }
        if (have)
        {
            reader->read.push_back(std::move(ready));
        }
    }
}

struct WarmProgram
{
    GLuint program = 0;
    uint64_t check = 0;
    std::chrono::steady_clock::duration made_time{};
};

// Programs made ahead, by entry file name, and a running average of the GL
// time of one (each new one weighs 1/8; the driver's first binary takes some
// 100 ms, once, which the cap keeps out).
std::unordered_map<std::string, WarmProgram> g_warm;
std::chrono::steady_clock::duration g_warm_typical = std::chrono::milliseconds(1);

}  // namespace

uint32_t LoadCachedProgram(const std::string& vertex_source, const std::string& fragment_source,
                           profile::ProgramOrigin* origin,
                           std::chrono::steady_clock::duration* made_time)
{
    if (!ProgramCacheEnabled())
    {
        return 0;
    }
    const std::filesystem::path path = EntryPath(vertex_source, fragment_source);
    GLuint program = 0;
    if (const auto it = g_warm.find(path.filename().string()); it != g_warm.end())
    {
        const WarmProgram warm = it->second;
        g_warm.erase(it);
        if (warm.check == SourceCheck(vertex_source, fragment_source))
        {
            program = warm.program;
            if (origin != nullptr)
            {
                *origin = profile::ProgramOrigin::kWarmed;
            }
            if (made_time != nullptr)
            {
                *made_time = warm.made_time;
            }
        }
        else
        {
            Gl().DeleteProgram(warm.program);  // a name collision: read the file
        }
    }
    if (program == 0)
    {
        const auto read_started = std::chrono::steady_clock::now();
        std::vector<char> data;
        Entry entry;
        if (!ReadFile(path, &data) || !ParseEntry(data, &entry) || entry.vertex_size != vertex_source.size() ||
            entry.fragment_size != fragment_source.size() ||
            std::memcmp(entry.vertex, vertex_source.data(), entry.vertex_size) != 0 ||
            std::memcmp(entry.fragment, fragment_source.data(), entry.fragment_size) != 0)
        {
            return 0;
        }
        program = MakeProgram(entry.format, entry.binary, entry.binary_size);  // 0: refused; compiled instead
        if (program != 0)
        {
            if (origin != nullptr)
            {
                *origin = profile::ProgramOrigin::kBinaryCache;
            }
            if (made_time != nullptr)
            {
                *made_time = std::chrono::steady_clock::now() - read_started;
            }
        }
    }
    if (program != 0 && g_reader != nullptr)
    {
        std::lock_guard<std::mutex> guard(g_reader->mutex);
        g_reader->touch.push_back(path);
        g_reader->wake.notify_one();
    }
    return program;
}

void WarmProgramCache(std::chrono::steady_clock::time_point deadline)
{
    using Clock = std::chrono::steady_clock;
    if (WarmLimit() == 0 || !ProgramCacheEnabled() || Clock::now() >= deadline)
    {
        return;
    }
    if (g_reader == nullptr)
    {
        g_reader = new WarmReader;
        std::thread(RunReader, g_reader, Cache().directory).detach();
    }
    for (;;)
    {
        const Clock::time_point started = Clock::now();
        if (started + g_warm_typical >= deadline)
        {
            return;
        }
        ReadyEntry entry;
        {
            std::lock_guard<std::mutex> guard(g_reader->mutex);
            if (g_reader->read.empty())
            {
                return;  // the reader is behind, or done
            }
            entry = std::move(g_reader->read.front());
            g_reader->read.pop_front();
            g_reader->wake.notify_one();
        }
        GLuint program = 0;
        if (g_warm.find(entry.name) == g_warm.end())
        {
            program = MakeProgram(entry.format, entry.binary.data(), entry.binary.size());
        }
        const Clock::duration elapsed = Clock::now() - started;
        if (program != 0)
        {
            g_warm[entry.name] = WarmProgram{program, entry.check, elapsed};
        }
        g_warm_typical += (std::min(elapsed, Clock::duration(std::chrono::milliseconds(10))) - g_warm_typical) / 8;
        profile::CountProgramWarm(program != 0, elapsed);
    }
}

bool StoreProgram(uint32_t program, const std::string& vertex_source, const std::string& fragment_source)
{
    if (!ProgramCacheEnabled())
    {
        return false;
    }
    GlFunctions& gl = Gl();
    GLint length = 0;
    gl.GetProgramiv(program, kGlProgramBinaryLength, &length);
    if (length <= 0)
    {
        return false;
    }
    std::vector<char> binary(static_cast<size_t>(length));
    GLsizei written = 0;
    GLenum format = 0;
    gl.GetProgramBinary(program, length, &written, &format, binary.data());
    if (written <= 0)
    {
        return false;
    }
    std::vector<char> out(kMagic, kMagic + 4);
    PutU32(&out, kLayoutVersion);
    PutU32(&out, format);
    PutBlock(&out, Cache().driver.data(), Cache().driver.size());
    PutBlock(&out, vertex_source.data(), vertex_source.size());
    PutBlock(&out, fragment_source.data(), fragment_source.size());
    PutBlock(&out, binary.data(), static_cast<size_t>(written));
    // Write beside the entry, then rename, so a reader never sees half a file.
    const std::filesystem::path path = EntryPath(vertex_source, fragment_source);
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    // Closed and checked before the rename: a flush failing in the
    // destructor would otherwise publish a truncated entry.
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file.write(out.data(), static_cast<std::streamsize>(out.size()));
    file.close();
    std::error_code error;
    const char* failed = nullptr;
    if (!file)
    {
        failed = "cannot be written";
    }
    else
    {
        std::filesystem::rename(temporary, path, error);
        failed = error ? "cannot be renamed" : nullptr;
    }
    if (failed != nullptr)
    {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        ReportOnce("shader cache store",
                   Format("shader cache entry %s %s%s%s; programs not stored are compiled again on the next run",
                          temporary.string().c_str(), failed, error ? ": " : "", error ? error.message().c_str() : ""));
        return false;
    }
    return true;
}

}  // namespace rexbox::hle::d3d8
