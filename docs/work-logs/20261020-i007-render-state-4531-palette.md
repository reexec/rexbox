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

## 이어서: Suspend, SetVertexBlendModelView

* `D3DDevice_Suspend`(XDK 4531, CT3 `000F9330`)는 이름과 달리 `CDevice_SetStateVB(0)`만 부른다(**확인됨**). 미뤄 둔 상태를 푸시 버퍼에 내보내는 일이고, 유일한 호출자는 타이틀의 푸시 버퍼 그리기 함수(`000A44C0`, 곧 `BeginPush`)다. HLE의 그리기는 그때의 상태를 읽으므로 `AcceptNone`으로 둔다. `D3DDevice_Resume`은 CT3에 없어 시그니처를 확인할 수 없으므로 더하지 않았다.
* 다음 정지(`D3D_MakeRequestedSpace_8`)의 원인은 XbSymbolDatabase가 4531에서 식별하지 못한 `D3DDevice_SetVertexBlendModelView(Count, pModelViews, pInverseModelViews, pProjectionViewport)`(CT3 `000F61E0`, stdcall 16바이트)였다. 이것이 lift된 채 하드웨어 푸시 버퍼(디바이스 `+0`/`+4`)를 쓰려 했다. reCT3가 `xdk_symbols_extra.json`으로 이름을 붙이고, rexbox HLE는 `SetModelView` 경로를 쓴다: 첫 모델뷰 = world × view, composite = 그 모델뷰 × 투영·뷰포트, Count 0이면 끈다. 여러 행렬의 블렌딩은 모델링하지 않고 한 번 보고한다.
* 결과: reCT3는 120초 동안 멈추지 않고 1500프레임 넘게 진행해 "PRESS START" 화면까지 간다(frame 1200, 1500). 배경 3D는 아직 그려지지 않는다. 보고: `vertex-shader program … register type 22 is not handled yet`, `fog is not drawn yet`, 사전 변환 정점의 텍스처 좌표 생성·변환, 512x512 렌더 타깃 텍스처.

## 이어서: 배경 3D

| 원인 | 조치 | 근거 |
|---|---|---|
| 정점 셰이더 레지스터 타입 22 | `D3DVSDT_NORMPACKED3`(`0x16`): 파서, GLSL에서 unsigned short 둘을 32비트로 합쳐 부호 확장, 속성 2×`GL_UNSIGNED_SHORT`, 즉시 모드 패킹 | 공개 XDK 형식(**추정**), 화면으로 뒷받침 |
| 마이크로코드 없는 셰이더(`no microcode`) | `LayoutFromDeclaration`: D3DVSDE 레지스터로 `VertexLayout`을 만들어 고정 기능 경로로 그린다(위치 오프셋, 압축 법선, 텍스처 좌표 빈칸). 고정 기능 키에 `packed_normal` | **확인됨**(그려짐) |
| 4531에 `D3D_g_Stream` 미식별 | reCT3 프로파일이 `0x00104ED8`(`CDevice_SetStateVB`가 `+stream×12`로 읽음)을 바인딩. rexbox는 `D3D_g_Stream` 기준 4134 오프셋(IndexData, dirty flags)을 4134 배치일 때만 쓴다 | **확인됨** |
| 푸시 버퍼 `0x1800`/`0x1808` | `ARRAY_ELEMENT16/32`를 모아 현재 스트림 0으로 인덱스 그리기(`DrawArrayElements`). 정점 배열은 `SetVertexShaderInput`의 스트림 | **확인됨**(타이틀 `000AAD20`) |
| 상태가 기본값 그대로(화면 전체를 덮는 그림자 사각형이 블렌드 꺼짐·스텐실 ALWAYS) | 4531 `SetRenderState_Simple`은 슬롯에 쓰지 않는다. NV2A 메서드 → 상태 표(라이브러리 `0x0012A590`, kb §7)로 HLE가 슬롯에 저장한다. `0x1D78`는 4531의 슬롯 82 | **확인됨** |

결과: d3d8 probe 201개(182 → 201), CTest 5/5. reCT3 프레임 1900·2100에 **어트랙트 데모의 3D 장면**(택시, 운전자, 밤의 도시, 미러볼)이 그려진다. 남은 보고: `fog is not drawn yet`, 여러 모델뷰의 블렌딩(첫 행렬만 씀).

## 다음

* 포그, 정점 블렌딩(`SetVertexBlendModelView`의 2개 이상 행렬), 사전 변환 정점의 텍스처 좌표 생성(#7).
* `D3D_g_Stream`이 없는 4531에서 `SetStreamSource`/`DrawVertices` 경로가 쓰이면 스트림 전역의 위치를 찾아야 한다.

---

# Work log: XDK 4531 render-state numbering, the default-state routine, palettes (issue #7)

Design: [20261020-i007-render-state-4531-palette](../design/20261020-i007-render-state-4531-palette.md) · Work order: [same name](../work-orders/20261020-i007-render-state-4531-palette.md)

Done: `d3d8_render_state.{h,cpp}` with `RenderStateLayout`, `DeriveRenderStateLayout`, `SlotFromLogical`/`LogicalFromSlot` and `RenderStateSlotCount` (pure functions); `d3d8_globals` derives the layout from the bound `D3D_g_DeferredRenderState`/`D3D_g_ComplexRenderState`, `RenderStateValue`/`SetRenderStateValue` take logical IDs and the new `RenderStateSlotValue`/`SetRenderStateSlot` take the title's numbers, the count constants gone; `SetRenderStateNotInline` handles `State` as a slot and decides ZBias with `LogicalFromSlot`. **A fix outside the design**: `Resolve()` settled the addresses only once all four variables (`D3D_g_pDevice`, `D3D_g_Stream`, `D3D_g_RenderState`, `D3D_g_DeferredTextureState`) were bound; XDK 4531 (CT3) has no identified `D3D_g_Stream`, so every read and write of the render state, texture state and device globals was dropped (a capture showed every state 0). It now settles once `D3D_g_RenderState` is bound and leaves missing variables at 0. **This was the main cause of CT3's black frames.** Palettes: `D3DDevice_CreatePalette` (`Common = (Size << 30) | 0x01030001`, `(256 >> Size) × 4` bytes of data, `ResourceKind::kPalette`) and `D3DDevice_SetPalette` (`PipelineState::palettes`, counted bindings); P8: `DecodePalettedLevel` (swizzled indices through the palette into RGBA8), `ResolveTexture(header, palette)` taking the stage's palette, with the cache re-uploading when the palette's address, Common or content hash changes; `rexbox_d3d8_probe` gains 19 layout, mapping and P8 checks (163 → 182), while `CreatePalette`/`SetPalette`, with no environment in the probe, were verified on reCT3; documents: kb `xbox-d3d8-api.md` §7 (the 4531 layout, palettes), `ARCHITECTURE.md`, the guides' probe counts.

Verification: the Win32 x86 Debug build (reCT3 tree) has no errors; the probes report kernel 83 / dsound 55 / d3d8 182 with 0 failures and reCT3's CTest passes 5/5; on reCT3 (hidden, the profile binding `D3D_InitializeDeviceState` = `0x000FAA20`) the `no D3D_InitializeDeviceState binding` report is gone, **frames 250, 500 and 800 show the title screen** (the Crazy Taxi 3 High Roller logo), and the run passes `CreatePalette`, presents over 900 frames and stops at `D3DDevice_Suspend` (`000F9330`). Whether CT3 actually samples P8 textures is not seen yet (no report from that path); that reJSRF (4134) still looks the same with this rexbox is checked on the user's machine (reJSRF still uses its own copy).

Then: XDK 4531's `D3DDevice_Suspend` (CT3 `000F9330`) only calls `CDevice_SetStateVB(0)` (**confirmed**), flushing deferred state into the push buffer for its one caller, the title's push-buffer draw (`000A44C0`, then `BeginPush`); draws here read the state when they happen, so it is `AcceptNone`; `D3DDevice_Resume` is absent from CT3 and its signature cannot be checked, so it was not added. The next stop (`D3D_MakeRequestedSpace_8`) came from `D3DDevice_SetVertexBlendModelView(Count, pModelViews, pInverseModelViews, pProjectionViewport)` (CT3 `000F61E0`, stdcall 16 bytes), which XbSymbolDatabase misses in 4531 and which, lifted, wrote the hardware push buffer (device `+0`/`+4`); reCT3 names it in `xdk_symbols_extra.json` and the rexbox HLE goes through `SetModelView`'s path (the first model view as world × view, the composite as that model view × projection-viewport, Count 0 turning it off; blending between matrices is not modelled and reported once). Result: reCT3 runs 120 s without a stop past 1500 frames to the PRESS START screen (frames 1200, 1500), its 3D background not drawn yet; reports: `vertex-shader program … register type 22 is not handled yet`, `fog is not drawn yet`, texture-coordinate generation/transform with pretransformed vertices, a 512x512 render-target texture.

Then, the 3D background: vertex-shader register type 22 is `D3DVSDT_NORMPACKED3` (`0x16`; parser, GLSL rebuilding the DWORD from two unsigned shorts and sign-extending each field, attributes as 2 × `GL_UNSIGNED_SHORT`, immediate packing; the format **inferred** from public XDK documents, borne out by the picture); a shader with no microcode is drawn on the fixed-function path through `LayoutFromDeclaration` (a `VertexLayout` from the D3DVSDE registers: position offset, packed normal, gaps in the texture sets; `packed_normal` in the fixed-function key); 4531 has no identified `D3D_g_Stream`, so the reCT3 profile binds `0x00104ED8` (read by `CDevice_SetStateVB` at `+stream×12`) and rexbox uses the 4134 offsets from `D3D_g_Stream` (IndexData, dirty flags) only for a 4134 layout; push-buffer `0x1800`/`0x1808` (`ARRAY_ELEMENT16/32`) are gathered and drawn indexed from the current stream 0 (`DrawArrayElements`; the arrays are `SetVertexShaderInput`'s streams, **confirmed** from the title's `000AAD20`); and states stayed at their defaults (a full-screen shadow quad covered the picture with blending off and stencil ALWAYS) because 4531's `SetRenderState_Simple` writes no slot, so the HLE now stores it from the NV2A method through the library's table (`0x0012A590`, kb §7; `0x1D78` is 4531's slot 82). Result: the d3d8 probe at 201 checks (182 → 201), CTest 5/5, and reCT3 frames 1900 and 2100 show **the attract demo's 3D scene** (taxi, driver, night city, mirror ball); still reported: `fog is not drawn yet` and blending between several model views (the first is used).

Next: fog, vertex blending (two or more matrices of `SetVertexBlendModelView`), texture-coordinate generation with pretransformed vertices (#7); if 4531 uses the `SetStreamSource`/`DrawVertices` path without `D3D_g_Stream`, the stream globals must be located.
