# 설계: render state로 쓴 combiner 상수 (issue #42)

상태: 구현됨([작업 지시](../work-orders/20261103-i042-combiner-factors.md), [작업 로그](../work-logs/20261103-i042-combiner-factors.md)). 소비자: reexec/reCT3#35. 이전 설계: reJSRF [i078](https://github.com/nworkers/reJSRF/blob/main/docs/design/20261017-i078-psh-definition-constants.md)(정의 상수와 `SetPixelShaderConstant`).

## 1. 요구사항

Crazy Taxi 3의 오프닝에서 바퀴 불꽃이 그려지지 않는다. 해변 장면을 캡처한 결과(**확인됨**):

* 불꽃 사각형은 택시 위치에 가산 블렌딩(SRCALPHA/ONE)으로 그려지지만 픽셀을 바꾸지 않는다. 텍스처(DXT3) 알파는 0이 아니다.
* 불꽃의 픽셀 셰이더는 2단계 combiner로, c0을 곱한다. `PS_COMBINERCOUNT`는 SAME_C0이고, 정의의 상수는 알파 0이다.
* 게임(`sub_000A4710`)은 그리기마다 `SetRenderState_Simple`로 NV2A `COMBINER_FACTOR0`(0xA60)와 `COMBINER_FACTOR1`(0xA80)을 쓴다. 논리 render state 10, 18이다. 하드웨어에서는 이 레지스터가 combiner가 읽는 상수다.
* HLE는 상수를 `SetPixelShaderConstant`(이 게임은 부르지 않음)와 정의에서만 가져온다.

## 2. 설계

* combiner 상수 render state: 단계별 `COMBINER_FACTOR0`(논리 10~17), `COMBINER_FACTOR1`(18~25), 최종 combiner의 `SPECULAR_FOG_FACTOR0/1`(43, 44). 값은 D3DCOLOR다.
* `SetRenderStateValue`가 이 상태를 쓰면 "SetPixelShader 이후 쓰임" 비트를 세운다. `SetPixelShader`(`ForgetPixelShaderConstants`)가 지운다. 상수 버전도 올려 uniform을 다시 만든다.
* `psh::StageConstants`: 정의 상수, `SetPixelShaderConstant` 매핑 다음에, 쓰인 factor가 있으면 그것을 쓴다. 단계 s의 c0은 UNIQUE_C0이면 factor0[s], 아니면 factor0[0]이다(c1도 같음).
* 원본 D3D8의 `SetPixelShader`는 정의의 상수를 같은 레지스터에 쓰므로, 그 뒤에 쓴 render state가 이기는 것은 하드웨어와 같다. `SetPixelShaderConstant`와 render state를 섞어 쓰는 타이틀에서는 마지막에 쓴 쪽을 가리지 않고 render state가 이긴다(**추정**: 드묾).

## 3. 검증

d3d8 probe(factor가 정의와 `SetPixelShaderConstant`보다 우선, 최종 상수, 쓰지 않은 factor는 정의), 소비자 reCT3의 불꽃(프레임 2155)과 데모 전체 비교, 사용자 확인.

---

# Design: combiner constants written as render states (issue #42)

Status: implemented ([work order](../work-orders/20261103-i042-combiner-factors.md), [work log](../work-logs/20261103-i042-combiner-factors.md)). Consumer: reexec/reCT3#35. Earlier design: reJSRF [i078](https://github.com/nworkers/reJSRF/blob/main/docs/design/20261017-i078-psh-definition-constants.md) (definition constants and `SetPixelShaderConstant`).

Requirement: Crazy Taxi 3's opening draws no wheel flames. Capturing the beach scene (**confirmed**): the flame quads are drawn at the taxi with additive blending (SRCALPHA/ONE) but change no pixel, their DXT3 alpha being non-zero; their pixel shader is a two-stage combiner multiplying by c0, `PS_COMBINERCOUNT` SAME_C0, the definition's constant alpha 0; the game (`sub_000A4710`) writes NV2A `COMBINER_FACTOR0` (0xA60) and `COMBINER_FACTOR1` (0xA80), logical render states 10 and 18, through `SetRenderState_Simple` before every draw, and on the hardware those registers are the constants the combiners read; the HLE took constants only from `SetPixelShaderConstant` (never called by this game) and the definition.

Design: the combiner constant render states are the per-stage `COMBINER_FACTOR0` (logical 10..17) and `COMBINER_FACTOR1` (18..25) and the final combiner's `SPECULAR_FOG_FACTOR0/1` (43, 44), as D3DCOLOR. `SetRenderStateValue` sets a "written since SetPixelShader" bit for them, which `SetPixelShader` (`ForgetPixelShaderConstants`) clears, and bumps the constants' version so the uniforms are formed again. `psh::StageConstants` takes a written factor after the definition and the `SetPixelShaderConstant` mapping: stage s's c0 is factor0[s] under UNIQUE_C0, else factor0[0] (c1 alike). The original `SetPixelShader` writes the definition's constants into the same registers, so a render state written after it winning matches the hardware; a title mixing `SetPixelShaderConstant` and these render states gets the render state whichever came last (**inferred** rare).

Verification: d3d8 probe (a factor overrides the definition and `SetPixelShaderConstant`, the final constants, unwritten factors keep the definition), the consumer reCT3's flames (frame 2155) and a whole-demo comparison, user check.
