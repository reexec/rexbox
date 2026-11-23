// Adapted from re2DJ (BSD 3-Clause License; see THIRD_PARTY_NOTICES.md).
// Copyright (c) 2026, re2DJ contributors

#include "d3d8_post_process.h"

#include "d3d8_gl.h"

#include "rexbox/platform/paths.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <mutex>

namespace rexbox::hle::d3d8
{

namespace
{

constexpr GLboolean kGlFalse = 0;
constexpr GLboolean kGlTrue = 1;

constexpr GLint kTextureUnit = 0;
constexpr GLint kOrigUnit = 1;
constexpr GLint kFirstPreviousUnit = 2;

constexpr GLfloat kUnitSquareProjection[16] = {
    2.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 1.0f,
};

constexpr const char* kStockShader = R"glsl(
#if defined(VERTEX)
attribute vec4 VertexCoord;
attribute vec4 TexCoord;
uniform mat4 MVPMatrix;
varying vec2 tex_coord;
void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    tex_coord = TexCoord.xy;
}
#elif defined(FRAGMENT)
uniform sampler2D Texture;
varying vec2 tex_coord;
void main()
{
    gl_FragColor = vec4(texture2D(Texture, tex_coord).rgb, 1.0);
}
#endif
)glsl";

std::mutex g_config_mutex;
std::filesystem::path g_configured_dir;
std::string g_initial_shader = kPostShaderNoneId;
D3d8PostProcess g_post_process;
bool g_initial_applied = false;

}  // namespace

struct D3d8PostProcess::Impl
{
    bool is_es = false;

    struct Pass
    {
        PostShaderPass settings;
        GLuint program = 0;
        GLint mvp_matrix = -1;
        GLint texture = -1;
        GLint input_size = -1;
        GLint texture_size = -1;
        GLint logical_size = -1;
        GLint output_size = -1;
        GLint frame_count = -1;
        GLint frame_direction = -1;
        GLint orig_texture = -1;
        GLint orig_texture_size = -1;
        GLint orig_input_size = -1;
        std::vector<std::array<GLint, 3>> previous;
        std::vector<GLint> parameter_locations;
        GLuint framebuffer = 0;
        GLuint color = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    bool initialized = false;
    GLuint quad_vao = 0;
    GLuint quad_vbo = 0;
    bool last_quad_flip_y = false;
    std::filesystem::path directory;
    std::string directory_text;
    std::vector<PostShaderEntry> catalog;
    std::string active_id = kPostShaderNoneId;
    std::string last_error;
    std::vector<PostShaderParameter> parameters;
    std::vector<Pass> passes;
    Pass stock;
    // A chain's copy of a top-down (flip_y) input, drawn upright, so that
    // every pass, OrigTexture and PassPrevN included, reads one orientation.
    Pass upright;
    std::uint32_t frames = 0;
};

namespace
{

using Pass = D3d8PostProcess::Impl::Pass;

std::string TrimLog(std::string log)
{
    while (!log.empty() && (log.back() == '\0' || log.back() == '\n' || log.back() == '\r'))
    {
        log.pop_back();
    }
    return log;
}

std::string ShaderLog(GLuint shader)
{
    GlFunctions& gl = Gl();
    GLint length = 0;
    gl.GetShaderiv(shader, kGlInfoLogLength, &length);
    std::string log(length > 0 ? static_cast<std::size_t>(length) : 0U, '\0');
    if (length > 0)
    {
        gl.GetShaderInfoLog(shader, length, nullptr, log.data());
    }
    return TrimLog(std::move(log));
}

std::string ProgramLog(GLuint program)
{
    GlFunctions& gl = Gl();
    GLint length = 0;
    gl.GetProgramiv(program, kGlInfoLogLength, &length);
    std::string log(length > 0 ? static_cast<std::size_t>(length) : 0U, '\0');
    if (length > 0)
    {
        gl.GetProgramInfoLog(program, length, nullptr, log.data());
    }
    return TrimLog(std::move(log));
}

GLuint CompileStage(GLenum type, const std::string& source, std::string* error)
{
    GlFunctions& gl = Gl();
    const GLuint shader = gl.CreateShader(type);
    if (shader == 0)
    {
        *error = "cannot create an OpenGL shader";
        return 0;
    }
    const char* text = source.c_str();
    gl.ShaderSource(shader, 1, &text, nullptr);
    gl.CompileShader(shader);
    GLint compiled = kGlFalse;
    gl.GetShaderiv(shader, kGlCompileStatus, &compiled);
    if (compiled != kGlTrue)
    {
        *error = std::string(type == kGlVertexShader ? "vertex" : "fragment") +
                 " stage failed to compile: " + ShaderLog(shader);
        gl.DeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint LinkProgram(GLuint vertex, GLuint fragment, std::string* error)
{
    GlFunctions& gl = Gl();
    const GLuint program = gl.CreateProgram();
    if (program == 0)
    {
        *error = "cannot create an OpenGL program";
        return 0;
    }
    gl.AttachShader(program, vertex);
    gl.AttachShader(program, fragment);
    gl.BindAttribLocation(program, D3d8PostProcess::kVertexCoordSlot, "VertexCoord");
    gl.BindAttribLocation(program, D3d8PostProcess::kColorSlot, "COLOR");
    gl.BindAttribLocation(program, D3d8PostProcess::kTexCoordSlot, "TexCoord");
    gl.LinkProgram(program);
    GLint linked = kGlFalse;
    gl.GetProgramiv(program, kGlLinkStatus, &linked);
    if (linked != kGlTrue)
    {
        *error = "link failed: " + ProgramLog(program);
        gl.DeleteProgram(program);
        return 0;
    }
    return program;
}

bool BuildPass(D3d8PostProcess::Impl* state,
               std::size_t index,
               Pass* pass,
               std::vector<PostShaderParameter>* parameters,
               std::string* error)
{
    PostShaderProgramSource source;
    if (!BuildPostShaderProgramSource(pass->settings.text, &source, error, state->is_es))
    {
        *error = pass->settings.path + ": " + *error;
        return false;
    }
    GlFunctions& gl = Gl();
    const GLuint vertex = CompileStage(kGlVertexShader, source.vertex, error);
    const GLuint fragment = vertex == 0 ? 0 : CompileStage(kGlFragmentShader, source.fragment, error);
    if (vertex != 0 && fragment != 0)
    {
        pass->program = LinkProgram(vertex, fragment, error);
    }
    if (vertex != 0)
    {
        gl.DeleteShader(vertex);
    }
    if (fragment != 0)
    {
        gl.DeleteShader(fragment);
    }
    if (pass->program == 0)
    {
        *error = pass->settings.path + ": " + *error;
        return false;
    }
    const auto uniform = [&gl, pass](const std::string& name)
    {
        return gl.GetUniformLocation(pass->program, name.c_str());
    };
    pass->mvp_matrix = uniform("MVPMatrix");
    pass->texture = uniform("Texture");
    pass->input_size = uniform("InputSize");
    pass->texture_size = uniform("TextureSize");
    pass->logical_size = uniform("LogicalSize");
    pass->output_size = uniform("OutputSize");
    pass->frame_count = uniform("FrameCount");
    pass->frame_direction = uniform("FrameDirection");
    pass->orig_texture = uniform("OrigTexture");
    pass->orig_texture_size = uniform("OrigTextureSize");
    pass->orig_input_size = uniform("OrigInputSize");
    for (std::size_t back = 1; back <= index + 1; ++back)
    {
        const std::string prefix = "PassPrev" + std::to_string(back);
        pass->previous.push_back(
            {uniform(prefix + "Texture"), uniform(prefix + "TextureSize"), uniform(prefix + "InputSize")});
    }
    for (PostShaderParameter& parameter : source.parameters)
    {
        bool known = false;
        for (const PostShaderParameter& existing : *parameters)
        {
            known = known || existing.name == parameter.name;
        }
        if (!known)
        {
            parameters->push_back(std::move(parameter));
        }
    }
    return true;
}

void ReleasePass(Pass* pass)
{
    GlFunctions& gl = Gl();
    if (pass->program != 0 && gl.DeleteProgram != nullptr)
    {
        gl.DeleteProgram(pass->program);
    }
    if (pass->framebuffer != 0 && gl.DeleteFramebuffers != nullptr)
    {
        gl.DeleteFramebuffers(1, &pass->framebuffer);
    }
    if (pass->color != 0 && gl.DeleteTextures != nullptr)
    {
        gl.DeleteTextures(1, &pass->color);
    }
    *pass = Pass{};
}

void ReleaseProgram(D3d8PostProcess::Impl* state)
{
    for (Pass& pass : state->passes)
    {
        ReleasePass(&pass);
    }
    state->passes.clear();
    ReleasePass(&state->stock);
    ReleasePass(&state->upright);
    state->parameters.clear();
    state->active_id = kPostShaderNoneId;
}

void SetFilter(GLuint unit, GLuint texture, bool linear)
{
    GlFunctions& gl = Gl();
    const GLint filter = static_cast<GLint>(linear ? kGlLinear : kGlNearest);
    gl.BindSampler(unit, 0);
    gl.BindTexture(kGlTexture2D, texture);
    gl.TexParameteri(kGlTexture2D, kGlTextureMinFilter, filter);
    gl.TexParameteri(kGlTexture2D, kGlTextureMagFilter, filter);
    gl.TexParameteri(kGlTexture2D, kGlTextureWrapS, static_cast<GLint>(kGlClampToEdge));
    gl.TexParameteri(kGlTexture2D, kGlTextureWrapT, static_cast<GLint>(kGlClampToEdge));
}

bool EnsureTarget(Pass* pass, std::uint32_t width, std::uint32_t height, std::string* error)
{
    if (pass->framebuffer != 0 && pass->width == width && pass->height == height)
    {
        return true;
    }
    GlFunctions& gl = Gl();
    if (pass->color == 0)
    {
        gl.GenTextures(1, &pass->color);
    }
    if (pass->framebuffer == 0)
    {
        gl.GenFramebuffers(1, &pass->framebuffer);
    }
    gl.BindTexture(kGlTexture2D, pass->color);
    gl.TexParameteri(kGlTexture2D, kGlTextureWrapS, static_cast<GLint>(kGlClampToEdge));
    gl.TexParameteri(kGlTexture2D, kGlTextureWrapT, static_cast<GLint>(kGlClampToEdge));
    gl.TexParameteri(kGlTexture2D, kGlTextureMinFilter, static_cast<GLint>(kGlNearest));
    gl.TexParameteri(kGlTexture2D, kGlTextureMagFilter, static_cast<GLint>(kGlNearest));
    gl.TexParameteri(kGlTexture2D, kGlTextureMaxLevel, 0);
    gl.TexImage2D(kGlTexture2D, 0, static_cast<GLint>(pass->settings.float_framebuffer ? kGlRgba16f : kGlRgba8),
                  static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, kGlRgba,
                  pass->settings.float_framebuffer ? kGlFloat : kGlUnsignedByte, nullptr);
    gl.BindFramebuffer(kGlFramebuffer, pass->framebuffer);
    gl.FramebufferTexture2D(kGlFramebuffer, kGlColorAttachment0, kGlTexture2D, pass->color, 0);
    if (gl.CheckFramebufferStatus(kGlFramebuffer) != kGlFramebufferComplete)
    {
        *error = pass->settings.path + ": cannot draw into a " + std::to_string(width) + "x" +
                 std::to_string(height) + (pass->settings.float_framebuffer ? " floating-point" : "") + " target";
        return false;
    }
    pass->width = width;
    pass->height = height;
    return true;
}

void SetUniforms(D3d8PostProcess::Impl* state,
                 const Pass& pass,
                 std::uint32_t input_width,
                 std::uint32_t input_height,
                 std::uint32_t logical_width,
                 std::uint32_t logical_height,
                 std::uint32_t output_width,
                 std::uint32_t output_height,
                 std::uint32_t orig_width,
                 std::uint32_t orig_height,
                 const std::vector<std::array<std::uint32_t, 2>>& previous_sizes)
{
    GlFunctions& gl = Gl();
    gl.UseProgram(pass.program);
    if (pass.mvp_matrix >= 0)
    {
        gl.UniformMatrix4fv(pass.mvp_matrix, 1, kGlFalse, kUnitSquareProjection);
    }
    const auto size = [&gl](GLint location, std::uint32_t width, std::uint32_t height)
    {
        if (location >= 0)
        {
            gl.Uniform2f(location, static_cast<GLfloat>(width), static_cast<GLfloat>(height));
        }
    };
    if (pass.texture >= 0)
    {
        gl.Uniform1i(pass.texture, kTextureUnit);
    }
    size(pass.input_size, input_width, input_height);
    size(pass.texture_size, input_width, input_height);
    size(pass.logical_size, logical_width, logical_height);
    size(pass.output_size, output_width, output_height);
    if (pass.orig_texture >= 0)
    {
        gl.Uniform1i(pass.orig_texture, kOrigUnit);
    }
    size(pass.orig_texture_size, orig_width, orig_height);
    size(pass.orig_input_size, orig_width, orig_height);
    for (std::size_t back = 0; back < pass.previous.size() && back < previous_sizes.size(); ++back)
    {
        if (pass.previous[back][0] >= 0)
        {
            gl.Uniform1i(pass.previous[back][0], kFirstPreviousUnit + static_cast<GLint>(back));
        }
        size(pass.previous[back][1], previous_sizes[back][0], previous_sizes[back][1]);
        size(pass.previous[back][2], previous_sizes[back][0], previous_sizes[back][1]);
    }
    if (pass.frame_count >= 0)
    {
        gl.Uniform1i(pass.frame_count, static_cast<GLint>(state->frames));
    }
    if (pass.frame_direction >= 0)
    {
        gl.Uniform1i(pass.frame_direction, 1);
    }
    for (std::size_t index = 0; index < pass.parameter_locations.size(); ++index)
    {
        if (pass.parameter_locations[index] >= 0)
        {
            gl.Uniform1f(pass.parameter_locations[index], state->parameters[index].value);
        }
    }
}

void DrawUnitSquareVao(D3d8PostProcess::Impl* state, bool flip_y)
{
    GlFunctions& gl = Gl();
    gl.BindVertexArray(state->quad_vao);
    gl.DrawArrays(kGlTriangleStrip, flip_y ? 4 : 0, 4);
    gl.BindVertexArray(0);
}

void InitQuadBuffers(D3d8PostProcess::Impl* state)
{
    // Vertices 0..3: standard bottom-up GL texture coordinates.
    // Vertices 4..7: flip_y for top-down inputs.
    static constexpr GLfloat kVertices[8][10] = {
        {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    };
    constexpr GLsizei kStride = static_cast<GLsizei>(sizeof(kVertices[0]));
    GlFunctions& gl = Gl();
    gl.GenVertexArrays(1, &state->quad_vao);
    gl.GenBuffers(1, &state->quad_vbo);
    gl.BindVertexArray(state->quad_vao);
    gl.BindBuffer(kGlArrayBuffer, state->quad_vbo);
    gl.BufferData(kGlArrayBuffer, static_cast<GLsizeiptr>(sizeof(kVertices)), kVertices, kGlStaticDraw);
    gl.EnableVertexAttribArray(D3d8PostProcess::kVertexCoordSlot);
    gl.VertexAttribPointer(D3d8PostProcess::kVertexCoordSlot, 4, kGlFloat, kGlFalse, kStride,
                           reinterpret_cast<const void*>(0));
    gl.EnableVertexAttribArray(D3d8PostProcess::kColorSlot);
    gl.VertexAttribPointer(D3d8PostProcess::kColorSlot, 4, kGlFloat, kGlFalse, kStride,
                           reinterpret_cast<const void*>(4 * sizeof(GLfloat)));
    gl.EnableVertexAttribArray(D3d8PostProcess::kTexCoordSlot);
    gl.VertexAttribPointer(D3d8PostProcess::kTexCoordSlot, 2, kGlFloat, kGlFalse, kStride,
                           reinterpret_cast<const void*>(8 * sizeof(GLfloat)));
    gl.BindVertexArray(0);
    gl.BindBuffer(kGlArrayBuffer, 0);
}

void PrepareDrawState()
{
    GlFunctions& gl = Gl();
    gl.Disable(kGlDepthTest);
    gl.Disable(kGlBlend);
    gl.Disable(kGlCullFace);
    gl.Disable(kGlStencilTest);
    gl.Disable(kGlScissorTest);
    gl.Disable(kGlPolygonOffsetFill);
    gl.ColorMask(1, 1, 1, 1);
    if (gl.PolygonMode != nullptr)
    {
        gl.PolygonMode(kGlFrontAndBack, kGlFill);
    }
}

}  // namespace

D3d8PostProcess::D3d8PostProcess() : impl_(std::make_unique<Impl>()) {}

D3d8PostProcess::~D3d8PostProcess() = default;

bool D3d8PostProcess::Initialize(const std::filesystem::path& shader_dir, bool is_es, std::string* message)
{
    Impl* state = impl_.get();
    if (state->initialized)
    {
        return true;
    }
    GlFunctions& gl = Gl();
    if (gl.CreateShader == nullptr || gl.BindAttribLocation == nullptr || gl.GenVertexArrays == nullptr ||
        gl.GenFramebuffers == nullptr)
    {
        if (message != nullptr)
        {
            *message = "OpenGL shader entry points are unavailable";
        }
        return false;
    }
    state->is_es = is_es;
    InitQuadBuffers(state);
    InvalidateGlStateCache();
    const std::string exe_dir = platform::ExecutableDirectory();
    state->directory = shader_dir.empty() ? ResolvePostShaderDirectory(std::filesystem::path(exe_dir)) : shader_dir;
    state->directory_text = state->directory.string();
    state->catalog = ListPostShaders(state->directory);
    state->initialized = true;
    return true;
}

void D3d8PostProcess::Shutdown()
{
    Impl* state = impl_.get();
    if (!state->initialized)
    {
        return;
    }
    GlFunctions& gl = Gl();
    ReleaseProgram(state);
    if (state->quad_vbo != 0 && gl.DeleteBuffers != nullptr)
    {
        gl.DeleteBuffers(1, &state->quad_vbo);
        state->quad_vbo = 0;
    }
    if (state->quad_vao != 0 && gl.DeleteVertexArrays != nullptr)
    {
        gl.DeleteVertexArrays(1, &state->quad_vao);
        state->quad_vao = 0;
    }
    state->initialized = false;
}

bool D3d8PostProcess::initialized() const
{
    return impl_->initialized;
}

bool D3d8PostProcess::active() const
{
    return !impl_->passes.empty();
}

const std::vector<PostShaderEntry>& D3d8PostProcess::catalog() const
{
    return impl_->catalog;
}

const std::string& D3d8PostProcess::shader_directory() const
{
    return impl_->directory_text;
}

const std::string& D3d8PostProcess::active_id() const
{
    return impl_->active_id;
}

const std::string& D3d8PostProcess::last_error() const
{
    return impl_->last_error;
}

std::vector<PostShaderParameter>& D3d8PostProcess::parameters()
{
    return impl_->parameters;
}

std::size_t D3d8PostProcess::pass_count() const
{
    return impl_->passes.size();
}

bool D3d8PostProcess::Select(std::string_view id)
{
    Impl* state = impl_.get();
    state->last_error.clear();
    if (!state->initialized)
    {
        state->last_error = "post-processing is not initialized";
        return false;
    }
    GlFunctions& gl = Gl();
    const auto finish = [&gl](bool result)
    {
        if (gl.BindFramebuffer != nullptr)
        {
            gl.BindFramebuffer(kGlFramebuffer, 0);
        }
        InvalidateGlStateCache();
        return result;
    };

    ReleaseProgram(state);
    if (id.empty() || id == kPostShaderNoneId)
    {
        return finish(true);
    }

    const PostShaderEntry* entry = FindPostShader(state->catalog, id);
    PostShaderPreset preset;
    if (entry == nullptr)
    {
        state->last_error = "no shader named '" + std::string(id) + "' in " + state->directory_text + " or built in";
        return finish(false);
    }
    if (!LoadPostShaderPreset(*entry, &preset, &state->last_error))
    {
        return finish(false);
    }
    const bool chained = preset.passes.size() > 1 || !preset.passes.front().to_window;
    std::vector<Pass> passes(preset.passes.size());
    std::vector<PostShaderParameter> parameters;
    bool built = true;
    for (std::size_t index = 0; built && index < passes.size(); ++index)
    {
        passes[index].settings = std::move(preset.passes[index]);
        built = BuildPass(state, index, &passes[index], &parameters, &state->last_error);
    }
    if (built && !passes.back().settings.to_window)
    {
        state->stock.settings = SinglePassPreset("stock", kStockShader).passes.front();
        built = BuildPass(state, passes.size(), &state->stock, &parameters, &state->last_error);
    }
    if (built && chained)
    {
        state->upright.settings = SinglePassPreset("upright", kStockShader).passes.front();
        state->upright.settings.to_window = false;
        state->upright.settings.float_framebuffer = false;
        built = BuildPass(state, passes.size() + 1, &state->upright, &parameters, &state->last_error);
    }
    state->passes = std::move(passes);
    if (!built)
    {
        const std::string error = state->last_error;
        ReleaseProgram(state);
        state->last_error = error;
        return finish(false);
    }
    state->parameters = std::move(parameters);
    const auto locate = [&gl, state](Pass* pass)
    {
        if (pass->program == 0)
        {
            return;
        }
        for (const PostShaderParameter& parameter : state->parameters)
        {
            pass->parameter_locations.push_back(gl.GetUniformLocation(pass->program, parameter.name.c_str()));
        }
    };
    for (Pass& pass : state->passes)
    {
        locate(&pass);
    }
    locate(&state->stock);
    state->active_id = std::string(id);
    state->frames = 0;
    return finish(true);
}

bool D3d8PostProcess::Reload()
{
    Impl* state = impl_.get();
    if (!state->initialized)
    {
        return false;
    }
    const std::string id = state->active_id;
    state->catalog = ListPostShaders(state->directory);
    return Select(id);
}

bool D3d8PostProcess::UseForPresent(std::uint32_t source_texture,
                                    std::uint32_t input_width,
                                    std::uint32_t input_height,
                                    std::uint32_t logical_width,
                                    std::uint32_t logical_height,
                                    std::int32_t viewport_x,
                                    std::int32_t viewport_y,
                                    std::uint32_t output_width,
                                    std::uint32_t output_height,
                                    bool flip_y)
{
    Impl* state = impl_.get();
    if (state->passes.empty())
    {
        return false;
    }
    GlFunctions& gl = Gl();
    PrepareDrawState();

    if (state->passes.size() == 1 && state->passes.front().settings.to_window)
    {
        const Pass& pass = state->passes.front();
        gl.ActiveTexture(kGlTexture0);
        // Single-pass shaders default to linear filtering (matching rexbox's
        // presentation blit) unless the preset specified otherwise.
        SetFilter(0, static_cast<GLuint>(source_texture),
                  pass.settings.filter_specified ? pass.settings.filter_linear : true);
        SetUniforms(state, pass, input_width, input_height, logical_width, logical_height,
                    output_width, output_height, input_width, input_height, {{input_width, input_height}});
        if (pass.orig_texture >= 0)
        {
            gl.Uniform1i(pass.orig_texture, kTextureUnit);
        }
        if (!pass.previous.empty() && pass.previous.front()[0] >= 0)
        {
            gl.Uniform1i(pass.previous.front()[0], kTextureUnit);
        }
        gl.BindFramebuffer(kGlFramebuffer, 0);
        gl.Viewport(viewport_x, viewport_y, static_cast<GLsizei>(output_width),
                    static_cast<GLsizei>(output_height));
        state->last_quad_flip_y = flip_y;
        ++state->frames;
        return true;
    }

    std::string error;
    const auto fail = [&]()
    {
        gl.BindFramebuffer(kGlFramebuffer, 0);
        gl.ActiveTexture(kGlTexture0);
        gl.BindTexture(kGlTexture2D, static_cast<GLuint>(source_texture));
        gl.Viewport(viewport_x, viewport_y, static_cast<GLsizei>(output_width),
                    static_cast<GLsizei>(output_height));
        ReleaseProgram(state);
        gl.UseProgram(0);
        while (gl.GetError() != kGlNoError)
        {
        }
        state->last_error = error;
        return false;
    };

    // A top-down input is drawn upright first: later passes read the
    // original (OrigTexture, PassPrevN) with their own texture coordinates,
    // which a flip in pass 0's quad alone would leave upside down.
    GLuint source = static_cast<GLuint>(source_texture);
    if (flip_y && state->upright.program != 0)
    {
        if (!EnsureTarget(&state->upright, input_width, input_height, &error))
        {
            return fail();
        }
        gl.ActiveTexture(kGlTexture0);
        SetFilter(0, source, false);
        SetUniforms(state, state->upright, input_width, input_height, logical_width, logical_height, input_width,
                    input_height, input_width, input_height, {{input_width, input_height}});
        gl.BindFramebuffer(kGlFramebuffer, state->upright.framebuffer);
        gl.Viewport(0, 0, static_cast<GLsizei>(input_width), static_cast<GLsizei>(input_height));
        DrawUnitSquareVao(state, true);
        source = state->upright.color;
        flip_y = false;
    }

    std::vector<GLuint> outputs = {source};
    std::vector<std::array<std::uint32_t, 2>> sizes = {{input_width, input_height}};
    gl.ActiveTexture(kGlTexture0 + static_cast<GLenum>(kOrigUnit));
    SetFilter(static_cast<GLuint>(kOrigUnit), outputs.front(), state->passes.front().settings.filter_linear);
    const std::size_t count = state->passes.size();
    for (std::size_t index = 0; index <= count; ++index)
    {
        const bool stock = index == count;
        if (stock && state->stock.program == 0)
        {
            break;
        }
        Pass& pass = stock ? state->stock : state->passes[index];
        const std::uint32_t pass_input_width = sizes.back()[0];
        const std::uint32_t pass_input_height = sizes.back()[1];
        std::uint32_t width = output_width;
        std::uint32_t height = output_height;
        const bool to_window = stock || pass.settings.to_window;
        if (!to_window)
        {
            PostShaderPassSize(pass.settings, pass_input_width, pass_input_height, output_width, output_height, &width,
                               &height);
            if (!EnsureTarget(&pass, width, height, &error))
            {
                return fail();
            }
        }
        for (std::size_t back = 1; back <= outputs.size() && back <= pass.previous.size(); ++back)
        {
            if (pass.previous[back - 1][0] >= 0)
            {
                const GLuint unit = static_cast<GLuint>(kFirstPreviousUnit) + static_cast<GLuint>(back - 1);
                gl.ActiveTexture(kGlTexture0 + unit);
                gl.BindSampler(unit, 0);
                gl.BindTexture(kGlTexture2D, outputs[outputs.size() - back]);
            }
        }
        gl.ActiveTexture(kGlTexture0 + static_cast<GLenum>(kOrigUnit));
        gl.BindSampler(static_cast<GLuint>(kOrigUnit), 0);
        gl.BindTexture(kGlTexture2D, outputs.front());
        gl.ActiveTexture(kGlTexture0);
        SetFilter(0, outputs.back(), stock || pass.settings.filter_linear);

        std::vector<std::array<std::uint32_t, 2>> previous_sizes;
        for (std::size_t back = 1; back <= sizes.size(); ++back)
        {
            previous_sizes.push_back(sizes[sizes.size() - back]);
        }
        SetUniforms(state, pass, pass_input_width, pass_input_height, logical_width, logical_height,
                    width, height, input_width, input_height, previous_sizes);
        if (to_window)
        {
            gl.BindFramebuffer(kGlFramebuffer, 0);
            gl.Viewport(viewport_x, viewport_y, static_cast<GLsizei>(output_width),
                        static_cast<GLsizei>(output_height));
            state->last_quad_flip_y = (index == 0) && flip_y;
            ++state->frames;
            return true;
        }
        gl.BindFramebuffer(kGlFramebuffer, pass.framebuffer);
        gl.Viewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        DrawUnitSquareVao(state, (index == 0) && flip_y);
        outputs.push_back(pass.color);
        sizes.push_back({width, height});
    }
    return false;
}

void D3d8PostProcess::DrawLastQuad()
{
    Impl* state = impl_.get();
    GlFunctions& gl = Gl();
    DrawUnitSquareVao(state, state->last_quad_flip_y);
    gl.ActiveTexture(kGlTexture0);
    gl.BindTexture(kGlTexture2D, 0);
    gl.UseProgram(0);
}

void SetPostShaderDirectory(const std::filesystem::path& directory)
{
    std::lock_guard<std::mutex> lock(g_config_mutex);
    g_configured_dir = directory;
}

std::filesystem::path GetPostShaderDirectory()
{
    std::lock_guard<std::mutex> lock(g_config_mutex);
    return g_configured_dir;
}

void SetInitialPostShader(std::string id)
{
    std::lock_guard<std::mutex> lock(g_config_mutex);
    g_initial_shader = id.empty() ? std::string(kPostShaderNoneId) : std::move(id);
}

D3d8PostProcess& EnsurePostProcessInitialized()
{
    if (!g_post_process.initialized())
    {
        std::filesystem::path dir;
        std::string initial;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            dir = g_configured_dir;
            initial = g_initial_shader;
        }
        std::string message;
        if (g_post_process.Initialize(dir, GlIsEs(), &message) && !g_initial_applied)
        {
            g_initial_applied = true;
            if (!initial.empty() && initial != kPostShaderNoneId)
            {
                if (!g_post_process.Select(initial))
                {
                    std::fprintf(stderr, "[d3d8] post shader '%s' not applied: %s\n",
                                 initial.c_str(), g_post_process.last_error().c_str());
                }
            }
        }
    }
    return g_post_process;
}

void ShutdownPostProcess()
{
    g_post_process.Shutdown();
    g_initial_applied = false;
}

PostShaderControl* GetPostShaderControl()
{
    return g_post_process.initialized() ? &g_post_process : nullptr;
}

}  // namespace rexbox::hle::d3d8
