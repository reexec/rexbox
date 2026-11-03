# 설계: XDK 5849 타이틀이 직접 닿는 HLE export와 LTCG 정적 디바이스 (issue #56)

상태: **구현됨**. 작업 로그: [20261103-i056](../work-logs/20261103-i056-title-reached-exports.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Pump It Up: Exceed는 HLE 대상으로 고른 함수 가운데 rexbox에 없는 27개를 직접 부른다(lift된 코드의 호출 지점과 생성된 override의 대조, **확인됨**). 또 인라인된 D3D 코드가 g_pDevice(`[0x00185A88]`)뿐 아니라 라이브러리의 정적 디바이스 `0x001835E0`을 직접 써서 푸시 버퍼에 쓴다(12곳, **확인됨**).

## 2. 설계

| 함수 | 처리 | 인자(lift 본문의 `ret`) |
|---|---|---|
| `D3DDevice_GetViewport`, `GetDisplayMode`, `GetCreationParameters` | HLE 상태(뷰포트, present 매개변수, 새로 기억하는 BehaviorFlags)로 답한다 | 1 |
| `GetRenderTarget2`, `GetDepthStencilSurface2` | 현재 표면, 참조 하나를 더해 eax로 | 0 |
| `GetVertexShader`, `GetPixelShader`, `GetShaderConstantMode` | setter가 저장한 값 | 1 |
| `GetScissors` | 개수 0, 배타 FALSE(시저는 저장하지 않는다) | 3 |
| `GetDisplayFieldStatus` | progressive(3, **추정**), vblank 수 | 1 |
| `GetVertexShaderConstant` | 상수 배열에서 | 3 |
| `SetVertexShaderConstantNotInline` | fastcall: ecx 레지스터, edx 데이터, 스택 개수 | 1 |
| `SetVertexShaderConstant1` | fastcall: ecx, edx, 하나 | 0 |
| `SetVertexShaderConstantNotInlineFast` | fastcall: ecx, edx, 스택 개수 | 1 |
| `PersistDisplay`, `D3D_DestroyResource_0__LTCG_edi1`, `D3D_UpdateProjectionViewportTransform` | 할 일 없음 | 0 |
| `EnableOverlay`, `SetRenderState_SampleAlpha` | 받기만 | 1 |
| `IDirectSoundBuffer_GetVoiceProperties`, `IDirectSoundStream_GetVoiceProperties` | DSVOICEPROPS 80바이트(**추정**)를 0으로 | 2 |
| `IDirectSoundBuffer_Set3DVoiceData`, `Use3DVoiceData`, `SetOutputBuffer` | 받기만(출력 버퍼 경로로 섞지 않는다) | 2 |
| `IDirectSoundBuffer_SetPlayRegion` | 받기만 | 3 |
| `IDirectSound_SynchPlayback` | 받기만 | 1 |

* 정적 디바이스: 타이틀이 XDK 내부 변수 `D3D_g_Device`를 바인딩하면 `CreateDevice`는 힙 블록 대신 그 객체를 HLE 디바이스로 쓴다. 이미지가 가진 그대로 둔다(0x3000바이트를 채우면 뒤따르는 라이브러리 전역이 지워진다). 푸시 버퍼 필드는 i051대로 쓴다.
* 받기만 하는 설정은 소리·그림에 반영되지 않는다(다음 단계에서 필요해지면 구현).

## 3. 검증

* CTest 7/7, probe 그대로.
* reEXCEED: 27개를 지나고, `D3D_g_Device` 바인딩으로 프레임 58장이 60 fps 간격으로 present된다.

---

# Design: HLE exports an XDK 5849 title reaches directly, and the static LTCG device (issue #56)

Status: **implemented**. Work log: [20261103-i056](../work-logs/20261103-i056-title-reached-exports.md). Consumer: reexec/reEXCEED#4.

Pump It Up: Exceed calls 27 HLE-selected functions directly that rexbox does not export (found by matching the lifted call sites with the generated overrides, **confirmed**), and its inlined D3D code writes the push buffer through the library's static device `0x001835E0` directly as well as through g_pDevice (`[0x00185A88]`; twelve sites, **confirmed**). The table above gives each function's handling and stack arguments (from the lifted bodies' `ret`): the D3D8 queries answer from the HLE's state (the viewport, the present parameters, the newly kept BehaviorFlags, the current surfaces with a reference added, the setters' shader handles and constant mode, no scissors since none are kept, a progressive field (3, **inferred**) with the vblank count, the constant array); the three vertex-constant setters are fastcalls (ecx register, edx data, and the count on the stack or one); `PersistDisplay`, `D3D_DestroyResource_0__LTCG_edi1` and `D3D_UpdateProjectionViewportTransform` have nothing to do; `EnableOverlay` and `SetRenderState_SampleAlpha` are accepted only; DSOUND's two `GetVoiceProperties` zero an 80-byte DSVOICEPROPS (**inferred**), and the 3D voice data, output buffer, play region and playback synchronization calls are accepted only (no submix routing). When the title binds the XDK internal `D3D_g_Device`, `CreateDevice` makes that object the HLE device instead of a heap block and leaves it as the image has it (filling 0x3000 bytes would wipe the library globals after it); the push-buffer fields follow i051. Accepted settings do not reach sound or picture. Verification: CTest 7/7 with the probes unchanged; reEXCEED passes the 27 and, with `D3D_g_Device` bound, presents 58 frames at 60 fps intervals.
