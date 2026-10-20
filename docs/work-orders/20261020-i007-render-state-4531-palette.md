# 작업 지시: XDK 4531 렌더 상태 번호, 기본 상태 루틴, 팔레트 (issue #7)

설계: [20261020-i007-render-state-4531-palette](../design/20261020-i007-render-state-4531-palette.md)

1. 작업 브랜치 `i008-physical-window`(진행 중인 브랜치에서 이어서). 커밋 제목 끝에 `(#7)`.
2. `d3d8_globals.{h,cpp}`: 바인딩된 `D3D_g_DeferredRenderState`/`D3D_g_ComplexRenderState`로 배치를 유도한다. `SlotFromLogical`/`LogicalFromSlot`, `RenderStateValue`/`SetRenderStateValue`(논리), `RenderStateSlotValue`/`SetRenderStateSlot`(슬롯). 개수 상수는 유도 값으로 바꾼다.
3. `d3d8_state.cpp` 등: 타이틀이 넘기는 상태 번호를 받는 곳(`SetRenderStateNotInline`, 그 밖에 있으면)을 슬롯 API로 바꾸고 ZBias 판단은 `LogicalFromSlot`으로 한다. `kRenderStateCount`를 쓰는 곳(캡처, 추적)을 점검한다.
4. 팔레트: `D3DDevice_CreatePalette`, `D3DDevice_SetPalette`, `PipelineState::palettes`, 리소스 표 등록.
5. P8: 업로드 때 스테이지 팔레트로 펼치기, 캐시 유효성에 팔레트 주소·크기·해시를 넣는다.
6. `rexbox_d3d8_probe`: 설계 §4의 검사.
7. reCT3(별도 커밋, reCT3 #3): `ct3_profile.cpp`의 `kXdkInternals`에 `{"D3D_InitializeDeviceState", 0x000FAA20}`, 분석 문서 갱신.
8. 문서: 작업 로그, `docs/kb/xbox-d3d8-api.md`(4531 렌더 상태 배치, 팔레트), `d3d8_render_state.h` 머리말.
9. 검증: 설계 §4.

---

# Work order: XDK 4531 render-state numbering, the default-state routine, palettes (issue #7)

Design: [20261020-i007-render-state-4531-palette](../design/20261020-i007-render-state-4531-palette.md)

(1) Continue on the task branch `i008-physical-window`; commit titles end with `(#7)`. (2) `d3d8_globals.{h,cpp}`: derive the layout from the bound `D3D_g_DeferredRenderState`/`D3D_g_ComplexRenderState`; `SlotFromLogical`/`LogicalFromSlot`, `RenderStateValue`/`SetRenderStateValue` (logical) and `RenderStateSlotValue`/`SetRenderStateSlot` (slots); the count constants become derived values. (3) `d3d8_state.cpp` and others: every place taking a state number from the title (`SetRenderStateNotInline`, any other) moves to the slot API, with ZBias decided by `LogicalFromSlot`; review the users of `kRenderStateCount` (capture, tracing). (4) Palettes: `D3DDevice_CreatePalette`, `D3DDevice_SetPalette`, `PipelineState::palettes`, registration in the resource table. (5) P8: expansion with the stage's palette at upload, the palette's address, size and hash in the cache's validity. (6) `rexbox_d3d8_probe`: the checks of design section 4. (7) reCT3 (a separate commit, reCT3 #3): `{"D3D_InitializeDeviceState", 0x000FAA20}` in `ct3_profile.cpp`'s `kXdkInternals`, and the analysis document. (8) Documents: the work log, `docs/kb/xbox-d3d8-api.md` (the 4531 render-state layout, palettes), the header comment of `d3d8_render_state.h`. (9) Verify as in design section 4.
