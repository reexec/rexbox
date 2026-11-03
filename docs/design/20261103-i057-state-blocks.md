# 설계: HLE 스냅숏으로 하는 상태 블록 (issue #57)

상태: **구현됨**. 작업 로그: [20261103-i057](../work-logs/20261103-i057-state-blocks.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Pump It Up: Exceed(XDK 5849)는 초기화 때 `CreateStateBlock(D3DSBT_ALL, &token)`을 한 번 부르고, 나중에 식별되지 않은 `ApplyStateBlock`(`0x00174BED`)으로 되돌린다. lift된 Create는 라이브러리 디바이스의 내부 필드(현재 정점 셰이더, 뷰포트 등)를 캡처하는데, HLE 디바이스는 그 필드를 채우지 않는다. 그래서 lift된 Apply가 정점 셰이더 0과 0x0 뷰포트를 되돌려 이후의 그리기가 모두 사라졌다(**확인됨**: 프레임 캡처에서 `vs 00000000`, `vp 0,0 0x0`, HLE 추적에서 `SetVertexShader(0)`, `SetViewport`가 `0x00174C30`/`0x00174D1C`, 즉 Apply 안에서 불림).

## 2. 설계

* 블록은 HLE 자신의 상태에 대한 호스트 스냅숏이다. 토큰은 게스트 힙의 작은 블록 주소(타이틀이 값으로만 들고 다닌다).
* 캡처 대상: 렌더 상태 전 슬롯(타이틀 번호), 텍스처 단계 상태 4 × 32, 스트림 16개(버퍼, stride), 파이프라인 전체(텍스처, 팔레트, 변환, 인덱스, 셰이더, 상수 모드 …), 뷰포트.
* `CreateStateBlock(Type, pToken)`: 형식과 관계없이 전부 캡처한다(픽셀·정점 블록에는 더 많이 담기는 셈, **추정**: 해가 없다). `ApplyStateBlock(Token)`: 되돌린다(텍스처 참조 수는 다시 세지 않는다). `CaptureStateBlock(Token)`, `DeleteStateBlock(Token)`.
* 타이틀은 Apply가 식별되지 않았으면 `xdk_symbols_extra.json`으로 이름을 준다(Exceed: `0x00174BED`, stdcall 4).

## 3. 검증

* CTest 7/7.
* reEXCEED: 프레임 캡처의 그리기 상태가 `vs 00000152`, `vp 0,0 640x480`이 되고, `vertex format 00000000` 보고가 사라진다.

---

# Design: state blocks as HLE snapshots (issue #57)

Status: **implemented**. Work log: [20261103-i057](../work-logs/20261103-i057-state-blocks.md). Consumer: reexec/reEXCEED#4.

Pump It Up: Exceed (XDK 5849) calls `CreateStateBlock(D3DSBT_ALL, &token)` once at start-up and later restores it with the unidentified `ApplyStateBlock` (`0x00174BED`). The lifted Create captures the library device's internal fields (the current vertex shader, the viewport, ...), which the HLE device does not fill, so the lifted Apply restored a null vertex shader and a 0x0 viewport and every later draw was dropped (**confirmed**: a frame capture shows `vs 00000000` and `vp 0,0 0x0`, and the HLE trace has `SetVertexShader(0)` and `SetViewport` called from `0x00174C30`/`0x00174D1C`, inside Apply).

A block is a host snapshot of the HLE's own state, keyed by a token that is a small guest heap block's address (the title only carries it as a value). It captures every render-state slot (the title's numbering), the texture-stage states (4 x 32), the 16 streams (buffer, stride), the whole pipeline (textures, palettes, transforms, indices, shaders, constant mode, ...) and the viewport. `CreateStateBlock(Type, pToken)` captures everything whatever the type (pixel and vertex blocks hold more than they would, **inferred** harmless); `ApplyStateBlock(Token)` restores it (texture reference counts are not replayed); `CaptureStateBlock(Token)` and `DeleteStateBlock(Token)` as named. A title whose Apply is unidentified names it in `xdk_symbols_extra.json` (Exceed: `0x00174BED`, stdcall 4).

Verification: CTest 7/7; in reEXCEED the frame capture's draw state becomes `vs 00000152` and `vp 0,0 640x480`, and the `vertex format 00000000` report is gone.
