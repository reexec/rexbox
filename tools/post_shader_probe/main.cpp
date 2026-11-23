// rexbox_post_shader_probe: verifies the presentation post-processing shader
// catalog, preset parser, GLSL 3.3 Core / GLES 3.0 source assembly (GL-free),
// and when a hidden OpenGL context is available, compiling all five built-ins,
// multi-pass chaining, error fallback, scanline logical-height scaling, and
// upright row order (issue #91, docs/design/20261124-i091-screen-shaders.md).

#include "d3d8_gl.h"
#include "d3d8_post_process.h"

#include "rexbox/hle/d3d8/d3d8_post_shader.h"
#include "rexbox/platform/display.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{

using namespace rexbox::hle::d3d8;

int g_checks = 0;
int g_failures = 0;

void Expect(bool condition, const std::string& what)
{
    ++g_checks;
    if (!condition)
    {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

void TestSourceAssemblyAndParameters()
{
    const char* raw =
        "#version 120\n"
        "#pragma parameter MY_GAIN \"Gain\" 1.25 0.0 4.0 0.05\n"
        "#pragma parameter NO_STEP \"No step\" 0.5 0.0 1.0\n"
        "#if defined(VERTEX)\n"
        "attribute vec4 VertexCoord;\n"
        "varying vec2 uv;\n"
        "void main() { gl_Position = VertexCoord; uv = vec2(0.0); }\n"
        "#elif defined(FRAGMENT)\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 uv;\n"
        "void main() {\n"
        "    vec4 my_gl_FragColor = texture2D(Texture, uv);\n"
        "    gl_FragColor = my_gl_FragColor;\n"
        "}\n"
        "#endif\n";

    PostShaderProgramSource src;
    std::string err;
    Expect(BuildPostShaderProgramSource(raw, &src, &err, false), "build GL 3.3 source");
    Expect(src.parameters.size() == 2, "two parameters parsed");
    if (src.parameters.size() == 2)
    {
        Expect(src.parameters[0].name == "MY_GAIN", "param 0 name");
        Expect(src.parameters[0].description == "Gain", "param 0 description");
        Expect(std::fabs(src.parameters[0].initial - 1.25f) < 0.001f, "param 0 initial");
        Expect(std::fabs(src.parameters[0].value - 1.25f) < 0.001f, "param 0 value");
        Expect(src.parameters[1].name == "NO_STEP", "param 1 name");
        Expect(src.parameters[1].step == 0.0f, "param 1 default step");
    }

    Expect(src.vertex.find("#version 330 core\n") == 0, "vertex version 330 core");
    Expect(src.vertex.find("#version 120") == std::string::npos, "vertex stripped #version 120");
    Expect(src.vertex.find("#define VERTEX\n") != std::string::npos, "vertex defines VERTEX");
    Expect(src.vertex.find("#define PARAMETER_UNIFORM\n") != std::string::npos, "vertex defines PARAMETER_UNIFORM");
    Expect(src.vertex.find("#define attribute in\n") != std::string::npos, "vertex attribute macro");
    Expect(src.vertex.find("#define varying out\n") != std::string::npos, "vertex varying macro");

    Expect(src.fragment.find("#version 330 core\n") == 0, "fragment version 330 core");
    Expect(src.fragment.find("#define FRAGMENT\n") != std::string::npos, "fragment defines FRAGMENT");
    Expect(src.fragment.find("#define varying in\n") != std::string::npos, "fragment varying macro");
    Expect(src.fragment.find("#define texture2D texture\n") != std::string::npos, "fragment texture2D macro");
    Expect(src.fragment.find("out vec4 rexbox_FragColor;\n") != std::string::npos, "fragment declares rexbox_FragColor");
    Expect(src.fragment.find(" gl_FragColor") == std::string::npos, "fragment replaced gl_FragColor");
    Expect(src.fragment.find("vec4 my_gl_FragColor") != std::string::npos, "fragment kept my_gl_FragColor");
    Expect(src.fragment.find("rexbox_FragColor = my_gl_FragColor;") != std::string::npos,
           "fragment assigned rexbox_FragColor");

    PostShaderProgramSource es_src;
    Expect(BuildPostShaderProgramSource(raw, &es_src, &err, true), "build GLES 3.0 source");
    Expect(es_src.fragment.find("#version 300 es\nprecision mediump float;\n") == 0, "fragment version 300 es");

    Expect(!BuildPostShaderProgramSource("", &src, &err), "reject empty source");
    Expect(!BuildPostShaderProgramSource("#if defined(VERTEX)\n#endif\n", &src, &err), "reject missing FRAGMENT");
    const char* bad_param =
        "#pragma parameter BAD \"Desc\" 1.0 2.0 0.5\n"
        "#if defined(VERTEX)\n#elif defined(FRAGMENT)\n#endif\n";
    Expect(!BuildPostShaderProgramSource(bad_param, &src, &err), "reject min > max parameter");
}

void TestPresetParsing()
{
    const char* preset_text =
        "\xEF\xBB\xBF"
        "# comment\n"
        "shaders = 2\n"
        "shader0 = pass0.glsl\n"
        "scale_type0 = source\n"
        "scale0 = 2.0\n"
        "filter_linear0 = false\n"
        "float_framebuffer0 = true\n"
        "shader1 = pass1.glsl\n";

    PostShaderPreset preset;
    std::string err;
    const auto reader = [](const std::string& rel, std::string* out, std::string* rerr) -> bool
    {
        (void)rerr;
        *out = "// " + rel + "\n#if defined(VERTEX)\n#elif defined(FRAGMENT)\n#endif\n";
        return true;
    };
    Expect(ParsePostShaderPreset(preset_text, reader, &preset, &err), "parse 2-pass preset");
    Expect(preset.passes.size() == 2, "2 passes");
    if (preset.passes.size() == 2)
    {
        Expect(preset.passes[0].scale_type_x == PostShaderScaleType::kSource, "pass 0 scale type source");
        Expect(std::fabs(preset.passes[0].scale_x - 2.0f) < 0.001f, "pass 0 scale 2.0");
        Expect(preset.passes[0].filter_specified && !preset.passes[0].filter_linear, "pass 0 nearest filter");
        Expect(preset.passes[0].float_framebuffer, "pass 0 float framebuffer");
        Expect(!preset.passes[0].to_window, "pass 0 offscreen");
        Expect(preset.passes[1].to_window, "pass 1 to window");

        std::uint32_t w = 0;
        std::uint32_t h = 0;
        PostShaderPassSize(preset.passes[0], 640, 480, 1280, 960, &w, &h);
        Expect(w == 1280 && h == 960, "pass 0 scaled size 1280x960");
        PostShaderPassSize(preset.passes[1], 1280, 960, 1280, 960, &w, &h);
        Expect(w == 1280 && h == 960, "pass 1 window size 1280x960");
    }

    Expect(!ParsePostShaderPreset("shaders = 0\n", reader, &preset, &err), "reject shaders = 0");
}

void TestCatalogAndPrecedence()
{
    std::error_code ec;
    const auto base = std::filesystem::temp_directory_path(ec) / "rexbox_post_shader_probe_catalog";
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(base, ec);

    const auto builtins = ListPostShaders(base / "nonexistent");
    Expect(builtins.size() == 5, "5 built-ins listed");
    if (builtins.size() == 5)
    {
        Expect(builtins[0].id == "crt" && builtins[0].builtin, "builtin 0 crt");
        Expect(builtins[1].id == "scanline" && builtins[1].builtin, "builtin 1 scanline");
        Expect(builtins[2].id == "fsr" && builtins[2].builtin, "builtin 2 fsr");
        Expect(builtins[3].id == "scalefx" && builtins[3].builtin, "builtin 3 scalefx");
        Expect(builtins[4].id == "anime4k" && builtins[4].builtin, "builtin 4 anime4k");
    }

    for (const auto& entry : builtins)
    {
        PostShaderPreset preset;
        std::string err;
        const bool loaded = LoadPostShaderPreset(entry, &preset, &err);
        Expect(loaded && !preset.passes.empty(), "load builtin " + entry.id);
        for (const auto& pass : preset.passes)
        {
            PostShaderProgramSource src;
            Expect(BuildPostShaderProgramSource(pass.text, &src, &err), "assemble " + entry.id + "/" + pass.path);
        }
    }

    {
        std::ofstream(base / "b.glsl") << "// b";
        std::ofstream(base / "a.glslp") << "// a";
        std::ofstream(base / "ignore.txt") << "// x";
    }
    const auto listed = ListPostShaders(base);
    Expect(listed.size() == 7, "5 built-ins + 2 user files");
    if (listed.size() == 7)
    {
        Expect(listed[5].id == "a.glslp" && !listed[5].builtin, "user file 0 a.glslp");
        Expect(listed[6].id == "b.glsl" && !listed[6].builtin, "user file 1 b.glsl");
    }

    const std::string cli = "crt";
    const std::string ini = "fsr";
    Expect(ChoosePostShader(&cli, "scanline", &ini) == "crt", "precedence CLI wins");
    Expect(ChoosePostShader(nullptr, "scanline", &ini) == "scanline", "precedence env wins");
    Expect(ChoosePostShader(nullptr, "", &ini) == "fsr", "precedence ini wins");
    Expect(ChoosePostShader(nullptr, nullptr, nullptr) == "none", "precedence default none");

    std::filesystem::remove_all(base, ec);
}

void TestGlPostProcess()
{
    constexpr uint32_t kLogicalW = 160;
    constexpr uint32_t kLogicalH = 120;
    constexpr uint32_t kOutW = kLogicalW * 2;
    constexpr uint32_t kOutH = kLogicalH * 2;

    rexbox::platform::DisplayConfig config;
    config.width = kOutW;
    config.height = kOutH;
    config.scale = 1;
    config.title = "rexbox_post_shader_probe";
    config.api = rexbox::platform::GlApi::kDesktopGl;
    config.hidden = true;
    config.vsync = rexbox::platform::VsyncMode::kOff;

    std::string error;
    if (!rexbox::platform::CreateDisplay(config, &error))
    {
        std::printf("SKIP GL checks (no display/GL context: %s)\n", error.c_str());
        return;
    }
    GlFunctions gl_loaded;
    std::string missing;
    if (!LoadGlFunctions(&gl_loaded, &missing))
    {
        std::printf("SKIP GL checks (missing GL function: %s)\n", missing.c_str());
        rexbox::platform::DestroyDisplay();
        return;
    }
    rexbox::platform::DisplayInfo info;
    rexbox::platform::GetDisplayInfo(&info);
    SetGlIsEs(info.gl_es);

    std::error_code ec;
    const auto shader_dir = std::filesystem::temp_directory_path(ec) / "rexbox_post_shader_probe_gl";
    std::filesystem::remove_all(shader_dir, ec);
    std::filesystem::create_directories(shader_dir, ec);

    {
        std::ofstream(shader_dir / "p0.glsl") <<
            "#if defined(VERTEX)\n"
            "attribute vec4 VertexCoord;\n"
            "attribute vec4 TexCoord;\n"
            "uniform mat4 MVPMatrix;\n"
            "varying vec2 uv;\n"
            "void main() { gl_Position = MVPMatrix * VertexCoord; uv = TexCoord.xy; }\n"
            "#elif defined(FRAGMENT)\n"
            "uniform sampler2D Texture;\n"
            "varying vec2 uv;\n"
            "void main() { vec4 c = texture2D(Texture, uv); gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); }\n"
            "#endif\n";
        std::ofstream(shader_dir / "p1.glsl") <<
            "#if defined(VERTEX)\n"
            "attribute vec4 VertexCoord;\n"
            "attribute vec4 TexCoord;\n"
            "uniform mat4 MVPMatrix;\n"
            "varying vec2 uv;\n"
            "void main() { gl_Position = MVPMatrix * VertexCoord; uv = TexCoord.xy; }\n"
            "#elif defined(FRAGMENT)\n"
            "uniform sampler2D Texture;\n"
            "varying vec2 uv;\n"
            "void main() { vec4 c = texture2D(Texture, uv); gl_FragColor = vec4(0.0, 0.0, c.g, 1.0); }\n"
            "#endif\n";
        std::ofstream(shader_dir / "two_pass.glslp") <<
            "shaders = 2\n"
            "shader0 = p0.glsl\n"
            "scale_type0 = source\n"
            "scale0 = 1.0\n"
            "shader1 = p1.glsl\n";
        std::ofstream(shader_dir / "orig.glsl") <<
            "#if defined(VERTEX)\n"
            "attribute vec4 VertexCoord;\n"
            "attribute vec4 TexCoord;\n"
            "uniform mat4 MVPMatrix;\n"
            "varying vec2 uv;\n"
            "void main() { gl_Position = MVPMatrix * VertexCoord; uv = TexCoord.xy; }\n"
            "#elif defined(FRAGMENT)\n"
            "uniform sampler2D Texture;\n"
            "uniform sampler2D OrigTexture;\n"
            "varying vec2 uv;\n"
            "void main() {\n"
            "    vec4 c = texture2D(Texture, uv);\n"
            "    gl_FragColor = vec4(texture2D(OrigTexture, uv).rgb + c.rgb * 0.0, 1.0);\n"
            "}\n"
            "#endif\n";
        std::ofstream(shader_dir / "orig_chain.glslp") <<
            "shaders = 2\n"
            "shader0 = p0.glsl\n"
            "scale_type0 = source\n"
            "scale0 = 1.0\n"
            "shader1 = orig.glsl\n";
        std::ofstream(shader_dir / "broken.glsl") <<
            "#if defined(VERTEX)\n"
            "void main() { broken_syntax_here }\n"
            "#elif defined(FRAGMENT)\n"
            "void main() { gl_FragColor = vec4(1.0); }\n"
            "#endif\n";
    }

    SetPostShaderDirectory(shader_dir);
    SetInitialPostShader("none");
    D3d8PostProcess& post = EnsurePostProcessInitialized();
    Expect(post.initialized(), "GL post-process initialized");
    Expect(GetPostShaderControl() == &post, "GetPostShaderControl returns active instance");

    // 1. All 5 built-ins compile and link on GL 3.3 Core.
    const char* kBuiltins[] = {"crt", "scanline", "fsr", "scalefx", "anime4k"};
    for (const char* id : kBuiltins)
    {
        const bool ok = post.Select(id);
        Expect(ok && post.active_id() == id,
               std::string("GL compile/link builtin ") + id + (ok ? "" : ": " + post.last_error()));
    }

    // 2. Broken shader falls back to none with an error message.
    Expect(!post.Select("broken.glsl"), "broken shader fails Select");
    Expect(post.active_id() == "none", "broken shader falls back to none");
    Expect(!post.last_error().empty(), "broken shader records last_error");

    // Create an upright (GL bottom-up) source texture of size kOutW x kOutH:
    // bottom half (GL rows 0..kOutH/2-1) = blue, top half (GL rows kOutH/2..kOutH-1) = red.
    GlFunctions& gl = Gl();
    std::vector<uint8_t> src_pixels(static_cast<size_t>(kOutW) * kOutH * 4u, 0);
    for (uint32_t y = 0; y < kOutH; ++y)
    {
        const bool top_half = y >= kOutH / 2;
        for (uint32_t x = 0; x < kOutW; ++x)
        {
            const size_t o = (static_cast<size_t>(y) * kOutW + x) * 4u;
            src_pixels[o + 0] = top_half ? 255 : 0;
            src_pixels[o + 1] = 0;
            src_pixels[o + 2] = top_half ? 0 : 255;
            src_pixels[o + 3] = 255;
        }
    }
    GLuint src_tex = 0;
    gl.GenTextures(1, &src_tex);
    gl.BindTexture(kGlTexture2D, src_tex);
    gl.TexParameteri(kGlTexture2D, kGlTextureMinFilter, static_cast<GLint>(kGlLinear));
    gl.TexParameteri(kGlTexture2D, kGlTextureMagFilter, static_cast<GLint>(kGlLinear));
    gl.TexParameteri(kGlTexture2D, kGlTextureMaxLevel, 0);
    gl.TexImage2D(kGlTexture2D, 0, static_cast<GLint>(kGlRgba8), static_cast<GLsizei>(kOutW),
                  static_cast<GLsizei>(kOutH), 0, kGlRgba, kGlUnsignedByte, src_pixels.data());

    const auto draw_and_read = [&](std::vector<uint8_t>* out)
    {
        gl.BindFramebuffer(kGlFramebuffer, 0);
        gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl.Clear(kGlColorBufferBit);
        const bool used = post.UseForPresent(src_tex, kOutW, kOutH, kLogicalW, kLogicalH, 0, 0, kOutW, kOutH, false);
        Expect(used, "UseForPresent succeeded for " + post.active_id());
        if (used)
        {
            post.DrawLastQuad();
        }
        out->assign(static_cast<size_t>(kOutW) * kOutH * 4u, 0);
        gl.BindFramebuffer(kGlReadFramebuffer, 0);
        gl.PixelStorei(kGlPackAlignment, 1);
        gl.ReadPixels(0, 0, static_cast<GLsizei>(kOutW), static_cast<GLsizei>(kOutH), kGlRgba, kGlUnsignedByte,
                      out->data());
    };

    const size_t bot_idx = (10u * kOutW + 40u) * 4u;
    const size_t top_idx = ((kOutH - 10u) * kOutW + 40u) * 4u;
    const size_t mid_idx = ((kOutH / 2u) * kOutW + (kOutW / 2u)) * 4u;

    // 3. Two-pass preset chains (pass 0 -> green, pass 1 -> blue).
    Expect(post.Select("two_pass.glslp"), "select two_pass.glslp");
    std::vector<uint8_t> fb;
    draw_and_read(&fb);
    Expect(fb[mid_idx + 0] <= 15 && fb[mid_idx + 1] <= 15 && fb[mid_idx + 2] >= 240,
           "two_pass output is blue");

    // 4. Single-pass scanline keeps upright orientation (top red, bottom blue)
    //    and darkens alternating rows against kLogicalH even at 2x input/output.
    Expect(post.Select("scanline"), "select scanline");
    draw_and_read(&fb);
    Expect(fb[top_idx + 0] >= 120 && fb[top_idx + 2] <= 20, "scanline top half stays red (upright)");
    Expect(fb[bot_idx + 0] <= 20 && fb[bot_idx + 2] >= 120, "scanline bottom half stays blue (upright)");
    const uint8_t row_a = fb[(20u * kOutW + 40u) * 4u + 2];
    const uint8_t row_b = fb[(21u * kOutW + 40u) * 4u + 2];
    Expect((row_a > row_b + 30) || (row_b > row_a + 30), "scanline modulates alternating rows at 2x");

    // 5. Multi-pass chain reading OrigTexture keeps upright orientation (top red, bottom blue).
    Expect(post.Select("orig_chain.glslp"), "select orig_chain.glslp");
    draw_and_read(&fb);
    Expect(fb[top_idx + 0] >= 240 && fb[top_idx + 2] <= 15, "orig_chain top half stays red (upright)");
    Expect(fb[bot_idx + 0] <= 15 && fb[bot_idx + 2] >= 240, "orig_chain bottom half stays blue (upright)");

    gl.DeleteTextures(1, &src_tex);
    ShutdownPostProcess();
    Expect(GetPostShaderControl() == nullptr, "GetPostShaderControl is null after shutdown");
    rexbox::platform::DestroyDisplay();
    std::filesystem::remove_all(shader_dir, ec);
}

}  // namespace

int main()
{
    TestSourceAssemblyAndParameters();
    TestPresetParsing();
    TestCatalogAndPrecedence();
    TestGlPostProcess();
    std::printf("%d check(s), %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
