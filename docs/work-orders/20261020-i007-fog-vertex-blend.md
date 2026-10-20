# 작업 지시: 포그와 정점 블렌딩 (issue #7, reexec/reCT3#5)

설계: [20261020-i007-fog-vertex-blend](../design/20261020-i007-fog-vertex-blend.md)

1. 작업 브랜치 `i007-fog-vertex-blend`(rexbox `main` v0.0.3에서). 커밋 제목 끝에 `(#7)`.
2. 순수 함수: `FogFromStates`, `FogFactor`, `SkinMatrices`(새 `d3d8_fog_blend.{h,cpp}` 또는 기존 상태 모듈).
3. 포그: 고정 기능 정점 셰이더의 포그 좌표(PLANAR, RADIAL, specular alpha, XYZRHW), 프래그먼트의 계수와 `mix`. 정점 프로그램의 `v_fog`를 같은 프래그먼트 식으로. 픽셀 셰이더 `FOG` 입력에 계수. 키(`fog_mode`, `fog_source`), uniform(`u_fog_params`, `u_fog_color`). `fog is not drawn yet` 보고 제거.
4. 블렌딩: `PipelineState`의 블렌드 행렬, `SetVertexBlendModelView`가 모두 보관. `VertexLayout::blend_offset`, `LayoutFromDeclaration`의 레지스터 1, FVF의 위치 뒤 가중치. 고정 기능 키(`blend_matrices`, `blend_implicit`), 셰이더의 합, uniform. world 행렬 블렌딩은 보고만.
5. `rexbox_d3d8_probe`: 설계 §4의 검사.
6. 문서: 작업 로그, kb `xbox-d3d8-api.md`(포그 상태와 NV2A 매핑, 스킨 모드), `ARCHITECTURE.md`의 probe 수.
7. reCT3(#5): submodule을 올리고, 분석 문서에 포그·블렌드 측정을 적고, 프레임 덤프로 비교한다.
8. 검증: 설계 §4.

---

# Work order: fog and vertex blending (issue #7, reexec/reCT3#5)

Design: [20261020-i007-fog-vertex-blend](../design/20261020-i007-fog-vertex-blend.md)

(1) Task branch `i007-fog-vertex-blend` from rexbox `main` v0.0.3; commit titles end with `(#7)`. (2) Pure functions `FogFromStates`, `FogFactor` and `SkinMatrices` (a new `d3d8_fog_blend.{h,cpp}` or the existing state module). (3) Fog: the fog coordinate in the fixed-function vertex shader (PLANAR, RADIAL, specular alpha, XYZRHW), the factor and `mix` in the fragment, vertex programs' `v_fog` through the same fragment formula, the factor in the pixel-shader `FOG` input, the keys (`fog_mode`, `fog_source`) and uniforms (`u_fog_params`, `u_fog_color`), and the `fog is not drawn yet` report removed. (4) Blending: the blend matrices in `PipelineState`, all kept by `SetVertexBlendModelView`; `VertexLayout::blend_offset`, register 1 in `LayoutFromDeclaration`, the FVF's weights after the position; the fixed-function key (`blend_matrices`, `blend_implicit`), the weighted sum in the shader and its uniforms; world-matrix blending only reported. (5) `rexbox_d3d8_probe`: the checks of design section 4. (6) Documents: the work log, kb `xbox-d3d8-api.md` (fog states and their NV2A mapping, skin modes), the probe count in `ARCHITECTURE.md`. (7) reCT3 (#5): move the submodule, record the fog and blend measurements in the analysis, compare frame dumps. (8) Verify as in design section 4.
