# 작업 로그: 표시 단계 화면 셰이더 (issue #91)

설계: [20261124-i091-screen-shaders.md](../design/20261124-i091-screen-shaders.md), 작업 지시: [20261124-i091-screen-shaders.md](../work-orders/20261124-i091-screen-shaders.md). 구현: Antigravity(`d2a3b19`). 설계·리뷰·검증: Claude.

## 경과

1. reEXCEED에 reDMP1의 화면 셰이더를 넣는 요청(reexec/reEXCEED#20)에서, 게임과 무관한 부분을 rexbox가 맡기로 했다. repsp v0.0.24(#50)를 참고 구현으로 삼았다.
2. 삽입 위치를 `PresentRenderTarget`(SMAA → 감마 램프 → 맞춘 사각형 blit → 오버레이)으로 정하고, 설계와 작업 지시를 썼다(`3653b67`).
3. Antigravity가 구현했다(37개 파일). GL 없는 목록·preset·소스 조립, `d3d8_post_process`, 내장 셰이더 5종, 제어 계약 `d3d8_post_shader.h`(앱 도우미 `ResolvePostShaderDirectory`, `ChoosePostShader` 포함), `rexbox_post_shader_probe`, 문서와 고지를 넣었다.

## 리뷰

* `none`이면 기존 경로를 그대로 탄다.
* 감마 램프가 항등이 아니면 입력 크기의 중간 텍스처를 거친다.
* 셰이더 경로가 실패하면 기존 blit으로 돌아간다.
* 끝나면 `InvalidateGlStateCache()`를 부르고 창 프레임버퍼를 다시 묶는다.
* 설계 밖의 추가로, 쓴 텍스처 단위의 샘플러 객체를 푼다(D3D8 샘플러 캐시가 셰이더 필터를 덮지 않게). 타당하다.
* `LogicalSize`는 `picture_width`(넓힌 폭 포함) × `source.height`다.

## 검증

| 항목 | 결과 |
|---|---|
| reEXCEED Win32 x86 Release, Debug | 경고 0, CTest 9/9(새 probe 포함) |
| `rexbox_post_shader_probe` | 93 checks, 0 failures(내장 5종 3.3 core 컴파일, 2 pass, 깨진 셰이더 `none` 복귀, 주사선, 위아래) |
| reEXCEED 실행 `--post-shader crt`/`none` | 타이틀 화면에 CRT(주사선·비네트·곡률)가 똑바로 적용되고 오버레이는 셰이더 밖, 60 fps. `none`은 이전과 같다 |

남은 확인(사용자): 다른 내장 셰이더, 16:9, 렌더 배율, MSAA·SMAA, 감마 램프 조합.

---

# Work log: screen shaders at presentation (issue #91)

Design and work order: [design](../design/20261124-i091-screen-shaders.md), [work order](../work-orders/20261124-i091-screen-shaders.md). Implemented by Antigravity (`d2a3b19`); designed, reviewed and verified by Claude.

## Progress

1. From the request to give reEXCEED reDMP1's screen shaders (reexec/reEXCEED#20), the game-independent part went to rexbox, with repsp v0.0.24 (#50) as the reference.
2. The hook point was set at `PresentRenderTarget` (SMAA, gamma ramp, fitted blit, overlay), and the design and work order were written (`3653b67`).
3. Antigravity implemented it in 37 files:
   * the GL-free catalog, preset and source assembly;
   * `d3d8_post_process` and the five built-ins;
   * the control contract `d3d8_post_shader.h`, with the app helpers `ResolvePostShaderDirectory` and `ChoosePostShader`;
   * `rexbox_post_shader_probe`, the documents and the notices.

## Review

* `none` takes the old path.
* A gamma ramp that is not the identity goes through an input-sized intermediate texture.
* A failing shader path falls back to the old blit.
* Afterwards `InvalidateGlStateCache()` runs and the window framebuffer is bound again.
* One addition outside the design: sampler objects are unbound on the units it uses, so the D3D8 sampler cache cannot override the shader filters. It is sound.
* `LogicalSize` is `picture_width` (with the widened width) by `source.height`.

## Verification

* reEXCEED Win32 x86 Release and Debug: 0 warnings, CTest 9/9 (with the new probe).
* `rexbox_post_shader_probe`: 93 checks, 0 failures (the five built-ins compile on 3.3 core, a 2-pass chain, a broken shader falls back to `none`, scanlines, upright).
* A reEXCEED run with `--post-shader crt` and with `none`:
  * `crt` shades the title screen upright (scanlines, vignette, curvature), the overlay stays unshaded, and it holds 60 fps.
  * `none` looks as before.

Left for the user: the other built-ins, 16:9, the render scale, MSAA and SMAA, and the gamma ramp.
