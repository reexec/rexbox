# 작업 로그: 포그와 정점 블렌딩 (issue #7, reexec/reCT3#5)

설계: [20261020-i007-fog-vertex-blend](../design/20261020-i007-fog-vertex-blend.md) · 작업 지시: [같은 이름](../work-orders/20261020-i007-fog-vertex-blend.md)

## 한 일

1. `d3d8_fog_blend.{h,cpp}`(새 파일): `FogFromStates`, `FogFactor`, `FogFunctionGlsl`, `SkinMatrices`.
2. 포그: 고정 기능 키에 `fog_mode`·`fog_source`. 정점 셰이더는 `v_fog`를 쓴다(PLANAR eye z, RADIAL 거리, NONE specular alpha, XYZRHW는 w). 프래그먼트는 `mix(u_fog_color.rgb, rgb, rexbox_fog(v_fog))`. 정점 프로그램의 `v_fog`(oFog)도 같은 프래그먼트 식을 거친다. 픽셀 셰이더 키에 `fog_mode`, `FOG` 레지스터의 alpha = 계수. uniform 위치(`u_fog_color`, `u_fog_params`)와 Draw의 설정. `fog is not drawn yet` 보고를 없앴다.
3. 정점 블렌딩: `PipelineState`에 `blend_count`, `blend_model_views[4]`, `projection_viewport`. `SetVertexBlendModelView`는 모두 보관하고, `SetModelView`는 블렌딩을 끈다. `VertexLayout`에 `blend_offset`·`blend_kind`(FVF `XYZB*`, declaration 레지스터 1). 고정 기능 키에 `blend_matrices`·`blend_weights`, 셰이더의 가중합과 법선 합, `attr::kBlendWeight`, uniform `u_blend_model_view[i]`·`u_blend_normal[i]`. 이때 변환은 `ScreenToClip(projection_viewport)`. world 행렬 블렌딩은 보고만 한다.
4. **설계에 없던 것**: CT3의 블렌드 가중치는 float가 아니라 `D3DVSDT_PBYTE4`(`0x44`, 정규화된 byte)다(**확인됨**: 처음 구현이 `blend weight type 68 is not float`로 메시를 거부했다). PBYTE와 NORMSHORT 가중치를 받고, GL에는 정규화된 `GL_UNSIGNED_BYTE`/`GL_SHORT`로 넘긴다.
5. `rexbox_d3d8_probe`: 포그 상태·계수(CT3의 EXP·density −0 → 1, LINEAR 음수 구간, start = end, EXP, EXP2), 스킨 모드 1..6, declaration·FVF의 가중치(PBYTE4 포함), 생성된 GLSL 검사 19개(201 → 220개).

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Debug 빌드(reCT3 트리) | 오류 0 |
| probe | kernel 83 / dsound 55 / d3d8 220, 0 failures |
| reCT3 `rect3.exe roms`(hidden, `REXBOX_D3D8_TRACE=1`) | 셰이더 오류와 "not drawn" 보고 없음. 만들어진 프로그램 키에 블렌딩 `;b21`(행렬 2개, 가중치 1개) 16, `;b32` 14, `;b43` 15, 포그 `;f30`(EXP, PLANAR) 311 |
| 데모 프레임 | 어트랙트 데모의 택시 점프, 밤 풍경, PRESS START 전환이 정상으로 그려진다. 데모 시점이 실행마다 달라 바꾸기 전후의 같은 장면은 비교하지 못했다 |

포그는 CT3 데모에서 계수 1(EXP, density −0)이라 화면 변화가 없다. 게임 플레이의 포그와 인물 스키닝은 사용자 환경에서 확인한다.

---

# Work log: fog and vertex blending (issue #7, reexec/reCT3#5)

Design: [20261020-i007-fog-vertex-blend](../design/20261020-i007-fog-vertex-blend.md) · Work order: [same name](../work-orders/20261020-i007-fog-vertex-blend.md)

Done: `d3d8_fog_blend.{h,cpp}` (new) with `FogFromStates`, `FogFactor`, `FogFunctionGlsl` and `SkinMatrices`; fog through `fog_mode`/`fog_source` in the fixed-function key, the vertex shader writing `v_fog` (PLANAR eye z, RADIAL distance, NONE specular alpha, w for XYZRHW), the fragment blending with `mix(u_fog_color.rgb, rgb, rexbox_fog(v_fog))`, vertex programs' `v_fog` (oFog) through the same fragment formula, `fog_mode` in the pixel-shader key with the factor in the `FOG` register's alpha, the uniform locations (`u_fog_color`, `u_fog_params`) set in Draw, and the `fog is not drawn yet` report gone; vertex blending through `blend_count`, `blend_model_views[4]` and `projection_viewport` in `PipelineState` (`SetVertexBlendModelView` keeps them all, `SetModelView` turns blending off), `blend_offset`/`blend_kind` in `VertexLayout` (FVF `XYZB*`, declaration register 1), `blend_matrices`/`blend_weights` in the fixed-function key with the weighted position and normal sums, `attr::kBlendWeight`, the uniforms `u_blend_model_view[i]`/`u_blend_normal[i]` and the transform `ScreenToClip(projection_viewport)`, world-matrix blending only reported. **Beyond the design**: CT3's blend weights are `D3DVSDT_PBYTE4` (`0x44`, normalized bytes), not floats (**confirmed**: the first implementation refused its meshes with `blend weight type 68 is not float`), so PBYTE and NORMSHORT weights are taken and given to GL as normalized `GL_UNSIGNED_BYTE`/`GL_SHORT`. `rexbox_d3d8_probe` gains 19 checks (201 → 220): fog states and factors (CT3's EXP with density −0 → 1, LINEAR over a negative range, start = end, EXP, EXP2), skin modes 1..6, weights in declarations and FVFs (PBYTE4 included), and the generated GLSL.

Verification: the Win32 x86 Debug build in the reCT3 tree has no errors; the probes report kernel 83 / dsound 55 / d3d8 220 with 0 failures; reCT3 `rect3.exe roms` (hidden, `REXBOX_D3D8_TRACE=1`) reports no shader error and no "not drawn", and among the programs built are the blending keys `;b21` (two matrices, one weight) 16 times, `;b32` 14, `;b43` 15, and the fog key `;f30` (EXP, PLANAR) 311 times; the attract demo's taxi jump, night view and PRESS START transition draw correctly, though the demo's timing differs between runs, so the same scene before and after could not be compared. Fog has factor 1 in CT3's demo (EXP, density −0) and changes nothing on screen; gameplay fog and character skinning are checked on the user's machine.
