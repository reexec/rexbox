# 작업 로그: XDK 4531 렌더 상태 번호, 기본 상태 루틴, 팔레트 (issue #7)

설계: [20261020-i007-render-state-4531-palette](../design/20261020-i007-render-state-4531-palette.md) · 작업 지시: [같은 이름](../work-orders/20261020-i007-render-state-4531-palette.md)

## 한 일

1. `d3d8_render_state.{h,cpp}`: `RenderStateLayout`, `DeriveRenderStateLayout`, `SlotFromLogical`/`LogicalFromSlot`, `RenderStateSlotCount`(순수 함수).
2. `d3d8_globals`: 바인딩된 `D3D_g_DeferredRenderState`/`D3D_g_ComplexRenderState`로 배치를 유도한다. `RenderStateValue`/`SetRenderStateValue`는 논리 ID를 받고, 새 `RenderStateSlotValue`/`SetRenderStateSlot`은 타이틀 번호를 받는다. 개수 상수를 없앴다.
3. `SetRenderStateNotInline`: `State`를 슬롯으로 다루고, ZBias는 `LogicalFromSlot`으로 판단한다.
4. **설계에 없던 수정**: `Resolve()`가 변수 네 개(`D3D_g_pDevice`, `D3D_g_Stream`, `D3D_g_RenderState`, `D3D_g_DeferredTextureState`)가 모두 바인딩되어야 주소를 확정했다. XDK 4531(CT3)에는 `D3D_g_Stream`이 식별되지 않아, 렌더 상태·텍스처 상태·디바이스 전역의 읽기와 쓰기가 전부 버려졌다(캡처에서 모든 상태가 0). 이제 `D3D_g_RenderState`가 바인딩되면 확정하고, 없는 변수는 0으로 둔다. **CT3의 검은 화면은 이것이 주된 원인이었다.**
5. 팔레트: `D3DDevice_CreatePalette`(`Common = (Size << 30) | 0x01030001`, 데이터 `(256 >> Size) × 4`바이트, `ResourceKind::kPalette`), `D3DDevice_SetPalette`(`PipelineState::palettes`, 바인딩 참조).
6. P8: `DecodePalettedLevel`(스위즐된 인덱스 → 팔레트 → RGBA8). `ResolveTexture(header, palette)`가 스테이지의 팔레트를 받고, 캐시는 팔레트의 주소·Common·내용 해시가 바뀌면 다시 올린다.
7. `rexbox_d3d8_probe`: 배치 유도와 변환, P8 검사 19개(163 → 182개). `CreatePalette`/`SetPalette`는 probe에 환경이 없어 reCT3 실행으로 검증했다.
8. 문서: kb `xbox-d3d8-api.md` §7(4531 배치, 팔레트), `ARCHITECTURE.md`, 가이드의 probe 수.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Debug 빌드(reCT3 트리) | 오류 0 |
| probe | kernel 83 / dsound 55 / d3d8 182, 0 failures. reCT3 CTest 5/5 |
| reCT3 `rect3.exe roms`(hidden, 프로파일이 `D3D_InitializeDeviceState` = `0x000FAA20`을 바인딩) | `no D3D_InitializeDeviceState binding` 보고가 사라짐. **frame 250, 500, 800에 타이틀 화면이 그려진다**(Crazy Taxi 3 High Roller 로고). `CreatePalette`를 지나 900프레임 넘게 돈 뒤 `D3DDevice_Suspend`(`000F9330`)에서 멈춘다 |

P8 텍스처가 실제로 CT3에서 쓰였는지는 아직 보지 못했다(그 경로의 보고가 없다). reJSRF(4134)에서 같은 rexbox로 화면이 그대로인지는 사용자 환경에서 확인한다(reJSRF는 아직 자체 사본을 쓴다).

## 다음

* `D3DDevice_Suspend`/`Resume`(동영상 재생 전후로 **추정**).
* `D3D_g_Stream`이 없는 4531에서 `SetStreamSource`/`DrawVertices` 경로가 쓰이면 스트림 전역의 위치를 찾아야 한다.

---

# Work log: XDK 4531 render-state numbering, the default-state routine, palettes (issue #7)

Design: [20261020-i007-render-state-4531-palette](../design/20261020-i007-render-state-4531-palette.md) · Work order: [same name](../work-orders/20261020-i007-render-state-4531-palette.md)

Done: `d3d8_render_state.{h,cpp}` with `RenderStateLayout`, `DeriveRenderStateLayout`, `SlotFromLogical`/`LogicalFromSlot` and `RenderStateSlotCount` (pure functions); `d3d8_globals` derives the layout from the bound `D3D_g_DeferredRenderState`/`D3D_g_ComplexRenderState`, `RenderStateValue`/`SetRenderStateValue` take logical IDs and the new `RenderStateSlotValue`/`SetRenderStateSlot` take the title's numbers, the count constants gone; `SetRenderStateNotInline` handles `State` as a slot and decides ZBias with `LogicalFromSlot`. **A fix outside the design**: `Resolve()` settled the addresses only once all four variables (`D3D_g_pDevice`, `D3D_g_Stream`, `D3D_g_RenderState`, `D3D_g_DeferredTextureState`) were bound; XDK 4531 (CT3) has no identified `D3D_g_Stream`, so every read and write of the render state, texture state and device globals was dropped (a capture showed every state 0). It now settles once `D3D_g_RenderState` is bound and leaves missing variables at 0. **This was the main cause of CT3's black frames.** Palettes: `D3DDevice_CreatePalette` (`Common = (Size << 30) | 0x01030001`, `(256 >> Size) × 4` bytes of data, `ResourceKind::kPalette`) and `D3DDevice_SetPalette` (`PipelineState::palettes`, counted bindings); P8: `DecodePalettedLevel` (swizzled indices through the palette into RGBA8), `ResolveTexture(header, palette)` taking the stage's palette, with the cache re-uploading when the palette's address, Common or content hash changes; `rexbox_d3d8_probe` gains 19 layout, mapping and P8 checks (163 → 182), while `CreatePalette`/`SetPalette`, with no environment in the probe, were verified on reCT3; documents: kb `xbox-d3d8-api.md` §7 (the 4531 layout, palettes), `ARCHITECTURE.md`, the guides' probe counts.

Verification: the Win32 x86 Debug build (reCT3 tree) has no errors; the probes report kernel 83 / dsound 55 / d3d8 182 with 0 failures and reCT3's CTest passes 5/5; on reCT3 (hidden, the profile binding `D3D_InitializeDeviceState` = `0x000FAA20`) the `no D3D_InitializeDeviceState binding` report is gone, **frames 250, 500 and 800 show the title screen** (the Crazy Taxi 3 High Roller logo), and the run passes `CreatePalette`, presents over 900 frames and stops at `D3DDevice_Suspend` (`000F9330`). Whether CT3 actually samples P8 textures is not seen yet (no report from that path); that reJSRF (4134) still looks the same with this rexbox is checked on the user's machine (reJSRF still uses its own copy).

Next: `D3DDevice_Suspend`/`Resume` (**inferred** to bracket movie playback); if 4531 uses the `SetStreamSource`/`DrawVertices` path without `D3D_g_Stream`, the stream globals must be located.
