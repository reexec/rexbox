# 표시 단계 화면 셰이더 작성·사용 안내

근거: [설계 i091](../design/20261124-i091-screen-shaders.md).

`rexbox_hle_d3d8`의 표시 경로(`PresentRenderTarget`)는 libretro 형식의 단일 패스 `.glsl` 셰이더와 다단계 `.glslp` 프리셋을 지원한다. 셰이더는 백 버퍼(SMAA·감마 램프 뒤)를 창에 그릴 때만 적용되며, 게임 로직·게스트 메모리·오버레이 메뉴·F9 캡처에는 영향을 주지 않는다.

## 1. 내장 셰이더와 사용자 디렉터리

* **내장 셰이더**: `none`, `crt`, `scanline`, `fsr`, `scalefx`, `anime4k`가 `rexbox_hle_d3d8`에 내장되어 있다.
* **사용자 디렉터리**: `ResolvePostShaderDirectory(executable_directory)`는 다음 순서로 `shaders/` 디렉터리를 고른다.
  1. 현재 작업 디렉터리의 `shaders/`가 있으면 그것.
  2. 없으면 `<executable_directory>/shaders/`가 있으면 그것.
  3. 둘 다 없으면 현재 작업 디렉터리의 `shaders/`를 반환한다.
* 목록은 내장 셰이더(고정 순서) 다음에 디렉터리의 `.glsl`, `.glslp` 파일 이름순으로 구성된다.

## 2. GLSL 3.3 Core / GLES 3.0 변환 규칙

libretro GLSL 셰이더는 보통 `#version 120` 문법(`attribute`, `varying`, `texture2D`, `gl_FragColor`)을 쓰지만, rexbox는 **OpenGL 3.3 Core**(GLES 문맥에서는 **OpenGL ES 3.0**)에서 실행된다. `BuildPostShaderProgramSource()`가 조립 시 자동으로 다음을 수행한다.

1. 소스 첫 줄의 `#version ...`을 제거하고 `#version 330 core`(GLES이면 `#version 300 es`와 `precision mediump float;`)를 붙인다.
2. `#define VERTEX` 또는 `#define FRAGMENT`, 그리고 `#define PARAMETER_UNIFORM`을 정의한다.
3. 호환 매크로를 주입한다.
   * 정점 단계: `#define attribute in`, `#define varying out`
   * 프래그먼트 단계: `#define varying in`, `#define texture2D texture`
4. 프래그먼트 소스에 `gl_FragColor` 토큰이 있으면 `out vec4 rexbox_FragColor;`를 선언하고 식별자 토큰 단위로 `gl_FragColor`를 `rexbox_FragColor`로 치환한다.

* 정점 속성 위치는 `VertexCoord = 0`, `COLOR = 1`, `TexCoord = 2`로 고정 바인딩된다.
* 텍스처 행 방향은 꼭짓점에서 맞추므로 셰이더는 위아래를 따로 뒤집지 않는다.

## 3. 크기 Uniform 계약: `LogicalSize`와 `TextureSize`

rexbox는 내부 렌더링 배율(`1x..8x`)과 Hor+ 와이드스크린을 지원한다.

| Uniform | 의미 |
|---|---|
| `TextureSize` | 입력 텍스처의 실제 픽셀 크기(렌더 배율·Hor+ 폭 포함) |
| `InputSize` | 해당 패스 입력 텍스처의 픽셀 크기 |
| `LogicalSize` | 게스트 그림 크기(`source.wide` 또는 `source.width` × `source.height`, 예: 640×480 또는 16:9 넓힌 폭) |
| `OutputSize` | 해당 패스 렌더 타깃(또는 최종 맞춘 사각형 뷰포트)의 픽셀 크기 |
| `OrigTexture` | 첫 패스 입력(원본 프레임) 샘플러 (`OrigTextureSize`, `OrigInputSize`) |
| `PassPrevNTexture` | `N`단계 이전 출력 샘플러 (`N = 1, 2, ...`, `PassPrevNTextureSize`, `PassPrevNInputSize`) |
| `FrameCount` | 누적 프레임 수 (`int`) |
| `FrameDirection` | 항상 `1` (`int`) |
| `MVPMatrix` | 정점 변환 행렬 (`mat4`) |

스캔라인·CRT처럼 렌더 배율이 아니라 게스트 원본 줄 수에 맞춰야 하는 셰이더는 `TextureSize` 대신 `uniform vec2 LogicalSize;`를 쓴다.

## 4. `#pragma parameter`와 `.glslp` 프리셋

* `#pragma parameter NAME "Label" initial min max [step]`: `uniform float NAME;`으로 전달되며 설정 메뉴에 슬라이더로 나타난다.
* `.glslp` 프리셋 키: `shaders`(1..16), `shaderN`, `scale_typeN`/`scale_type_xN`/`scale_type_yN`(`source`, `viewport`, `absolute`), `scaleN`/`scale_xN`/`scale_yN`, `filter_linearN`(`true`/`false`), `float_framebufferN`(`true`이면 `RGBA16F`, 아니면 `RGBA8`).
* 로드·컴파일·링크·타깃 생성이 실패하면 화면을 검게 만들지 않고 `none`으로 그리며 오류 문자열을 `PostShaderControl::last_error()`와 로그에 남긴다.

---

# Presentation Screen Shaders Guide

Based on: [design i091](../design/20261124-i091-screen-shaders.md).

`rexbox_hle_d3d8`'s presentation path (`PresentRenderTarget`) supports libretro-style single-pass `.glsl` shaders and multi-pass `.glslp` presets. Shaders apply only when presenting the back buffer (after SMAA and any gamma ramp) to the window; game logic, guest memory, the overlay menu and F9 captures are untouched.

## 1. Built-in Shaders and User Directory

* **Built-ins**: `none`, `crt`, `scanline`, `fsr`, `scalefx` and `anime4k` are embedded in `rexbox_hle_d3d8`.
* **User directory**: `ResolvePostShaderDirectory(executable_directory)` picks `shaders/` in the working directory when it exists, otherwise `<executable_directory>/shaders/` when it exists, falling back to `shaders/` in the working directory.
* The catalog lists the built-ins in a fixed order followed by `.glsl` and `.glslp` files in the directory sorted by name.

## 2. GLSL 3.3 Core / GLES 3.0 Translation

`BuildPostShaderProgramSource()` translates libretro `#version 120` style shaders at assembly time:

1. Drops any leading `#version` line and prepends `#version 330 core` (or `#version 300 es` + `precision mediump float;` for GLES).
2. Defines `VERTEX` or `FRAGMENT`, and `PARAMETER_UNIFORM`.
3. Injects `attribute`/`varying`/`texture2D` compatibility macros.
4. Token-replaces `gl_FragColor` with a declared `out vec4 rexbox_FragColor;` when `gl_FragColor` is used.

Vertex attributes are bound to `VertexCoord = 0`, `COLOR = 1` and `TexCoord = 2`. Orientation is handled in the vertices, so shaders do not flip Y.

## 3. Size Uniforms: `LogicalSize` vs `TextureSize`

* `TextureSize`, `InputSize`: real pixel size of the input texture (including render scale and Hor+ widened width).
* `LogicalSize`: guest picture size (`source.wide` or `source.width`, by `source.height`; e.g., 640x480 or widened 16:9 width). Use `LogicalSize` for scanline/CRT effects tied to the guest pixel grid rather than render scale.
* `OutputSize`, `OrigTexture` (`OrigTextureSize`, `OrigInputSize`), `PassPrevNTexture` (`N = 1, 2, ...`), `FrameCount`, `FrameDirection` (1), `MVPMatrix`, and `#pragma parameter` float uniforms.

## 4. `#pragma parameter` and `.glslp` Presets

* `#pragma parameter NAME "Label" initial min max [step]` exposes a float uniform as a slider in the settings menu.
* `.glslp` keys: `shaders` (1..16), `shaderN`, `scale_typeN`/`scale_type_xN`/`scale_type_yN` (`source`, `viewport`, `absolute`), `scaleN`/`scale_xN`/`scale_yN`, `filter_linearN`, `float_framebufferN` (`RGBA16F` when true, `RGBA8` otherwise).
* Any load, compile, link or target failure falls back to `none` and records the reason in `PostShaderControl::last_error()` and the log.
