# 작업 지시: 표시 단계 화면 셰이더 (issue #91)

설계: [20261124-i091-screen-shaders.md](../design/20261124-i091-screen-shaders.md). 구현: Antigravity. 리뷰·검증: Claude.

* 브랜치: rexbox `i091-screen-shaders`(이미 있음, `origin/main` v0.0.32 기준). 커밋 제목 끝에 `(#91)`. push, 버전, 태그는 하지 않는다.
* 참고 원본(읽기만, 고치지 않음): `E:\MYWORK\Projects\reexec\reDMP1\third_party\repsp`(repsp v0.0.24). 파일 목록은 repsp 커밋 `9c593ed`, `e01d630`, `f86b2e4`, `7bb5c5f`(`git -C ... show --stat`).

## 단계

1. **GL 없는 부분**: repsp `include/repsp/platform/post_shader_{catalog,preset,source}.h`, `src/platform/post_shader_{catalog,preset,source}.cpp`를 `include/rexbox/hle/d3d8/`, `src/hle/d3d8/`로 옮긴다. 네임스페이스는 `rexbox::hle::d3d8`. re2DJ 고지 주석은 그대로 둔다.
2. **내장 셰이더**: repsp `src/platform/post_shaders/` 전체를 `src/hle/d3d8/post_shaders/`로 옮기고, repsp `CMakeLists.txt`의 `post_shader_builtins.inc.in` 생성 방식을 rexbox D3D8 타깃에 맞게 넣는다(생성 파일은 빌드 디렉터리에만).
3. **GL pass**: repsp `src/platform/sdl/opengl_post_process.cpp`를 `src/hle/d3d8/d3d8_post_process.{h,cpp}`로 옮긴다. GL 호출은 `d3d8_gl.h`의 `GlFunctions`로 바꾸고, 없는 GL 함수는 `d3d8_gl.cpp`의 로더에 더한다. GLES 문맥(`platform::DisplayIsEs`)이면 `#version 300 es`.
4. **끼워 넣기**: `d3d8_gl_targets.cpp`의 `PresentRenderTarget`에 설계 3절대로 넣는다.
   * 셰이더 `none`이면 지금 코드 경로를 그대로 탄다.
   * 셰이더가 있고 감마 램프가 항등이 아니면 램프를 중간 텍스처에 먼저 그린다(`DrawThroughRamp`를 텍스처 대상으로 쓰거나 같은 셰이더로).
   * 마지막 pass는 맞춘 사각형 viewport에 그린다.
   * pass 뒤 `InvalidateGlStateCache()`를 부르고, 창 프레임버퍼(0)를 그리기 대상으로 다시 묶은 뒤 `platform::RenderOverlay()`로 넘어간다.
5. **크기 uniform**: 설계 4절(`LogicalSize` = `source.wide` 또는 `source.width` × `source.height`).
6. **제어 계약**: `include/rexbox/hle/d3d8/d3d8_post_shader.h`, 설계 6절. `SetInitialPostShader`는 첫 present에서 적용하고 실패를 로그에 남긴다. `GetPostShaderControl()`은 GL 장치가 없으면 null. repsp의 앱용 도우미 `ResolvePostShaderDirectory(exe_dir)`(작업 디렉터리의 `shaders/`, 없으면 실행 파일 옆)와 `ChoosePostShader(cli, env, file)`(명령줄 > 환경 변수 > 파일 > `none`)도 같은 헤더에 옮긴다.
7. **시험**: repsp의 GL 없는 시험과 `tools/post_shader_probe`를 rexbox 방식으로 옮겨 CTest에 등록한다(설계 8절 1, 2). 행 방향(위아래) 확인을 probe에 넣는다.
8. **문서**: `ARCHITECTURE.md`(한국어 먼저, 영어), `docs/guides/post-process-shaders.md`(repsp `docs/post-process-shaders.md`를 rexbox에 맞게, 한국어 먼저), `THIRD_PARTY_NOTICES.md`(repsp의 re2DJ·FSR·ScaleFX·Anime4K 항목과 BSD 전문). 소스 주석은 영어만.
9. **빌드·시험**: reEXCEED 저장소 루트에서 `cmake --build build/win32_x86 --config Release`(그리고 `Debug`), `ctest -C Release`(그리고 `Debug`)를 `build/win32_x86`에서 실행해 경고 0, 전부 통과를 확인한다. 새 시험이 CTest 목록에 나오는지 본다.

## 보고

끝나거나 막히면 `%TEMP%\reexceed-coder\i091-screen-shaders.txt`에 바꾼 파일, 빌드·시험 결과 요약, 커밋 id, 설계와 다르게 한 점과 질문을 쓰고 턴을 끝낸다.

---

# Work order: screen shaders at presentation (issue #91)

Design: [20261124-i091-screen-shaders.md](../design/20261124-i091-screen-shaders.md). Implementation: Antigravity. Review and verification: Claude.

* Branch: rexbox `i091-screen-shaders` (exists, from `origin/main` v0.0.32). Commit titles end with `(#91)`. No push, version or tag.
* Reference source (read only): `E:\MYWORK\Projects\reexec\reDMP1\third_party\repsp` (repsp v0.0.24). Its files are listed by repsp commits `9c593ed`, `e01d630`, `f86b2e4`, `7bb5c5f` (`git -C ... show --stat`).

## Steps

1. **GL-free parts**: move repsp `include/repsp/platform/post_shader_{catalog,preset,source}.h` and `src/platform/post_shader_{catalog,preset,source}.cpp` to `include/rexbox/hle/d3d8/` and `src/hle/d3d8/`, namespace `rexbox::hle::d3d8`, keeping the re2DJ notice comments.
2. **Built-ins**: move all of repsp `src/platform/post_shaders/` to `src/hle/d3d8/post_shaders/`, and bring repsp's `post_shader_builtins.inc.in` generation from its `CMakeLists.txt` into the rexbox D3D8 target (the generated file only in the build directory).
3. **GL pass**: move repsp `src/platform/sdl/opengl_post_process.cpp` to `src/hle/d3d8/d3d8_post_process.{h,cpp}`. Call GL through `GlFunctions` in `d3d8_gl.h`, adding missing functions to the loader in `d3d8_gl.cpp`. In a GLES context (`platform::DisplayIsEs`), use `#version 300 es`.
4. **Hook**: put it into `PresentRenderTarget` in `d3d8_gl_targets.cpp` as design section 3 says.
   * With the shader `none`, today's code path runs unchanged.
   * With a shader and a gamma ramp that is not the identity, draw the ramp into an intermediate texture first (`DrawThroughRamp` aimed at a texture, or the same shader).
   * Draw the last pass into the fitted rectangle's viewport.
   * After the passes, call `InvalidateGlStateCache()` and bind the window framebuffer (0) for drawing again before `platform::RenderOverlay()`.
5. **Size uniforms**: design section 4 (`LogicalSize` = `source.wide` or `source.width` by `source.height`).
6. **Control contract**: `include/rexbox/hle/d3d8/d3d8_post_shader.h`, design section 6. `SetInitialPostShader` applies at the first present and logs a failure; `GetPostShaderControl()` is null without a GL device. Also move repsp's app helpers `ResolvePostShaderDirectory(exe_dir)` (the working directory's `shaders/`, else next to the executable) and `ChoosePostShader(cli, env, file)` (command line > environment > file > `none`) into the same header.
7. **Tests**: move repsp's GL-free tests and `tools/post_shader_probe` into rexbox's style and register them with CTest (design section 8, items 1 and 2), including a row-order (upright) check in the probe.
8. **Documents**:
   * `ARCHITECTURE.md`, Korean first, then English.
   * `docs/guides/post-process-shaders.md`: repsp's `docs/post-process-shaders.md` adapted to rexbox, Korean first.
   * `THIRD_PARTY_NOTICES.md`: repsp's re2DJ, FSR, ScaleFX and Anime4K entries and the BSD text.
   * Source comments in English only.
9. **Build and test**: from the reEXCEED repository root, run `cmake --build build/win32_x86 --config Release` (and `Debug`), then `ctest -C Release` (and `Debug`) in `build/win32_x86`. Expect 0 warnings and all tests passing, and check that the new tests appear in the CTest list.

## Report

When done or stuck, write the changed files, a build and test summary, the commit id, anything done differently from the design, and questions to `%TEMP%\reexceed-coder\i091-screen-shaders.txt`, then end the turn.
