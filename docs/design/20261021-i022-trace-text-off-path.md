# 설계: 추적이 꺼져 있을 때 D3D8 추적 문장을 만들지 않는다 (issue #22)

## 1. 요구사항

소비자 reexec/reCT3#17: Crazy Taxi 3의 운전 화면(프레임당 그리기 약 810개, 42 ms)에서 표시 스레드를 샘플링하면 ucrtbase의 printf 내부(서식 해석 상태 기계, 실수 자릿수 생성과 그것이 부르는 `fegetround`, `_aulldiv`)와 문자열 할당이 표본의 약 8~9%, 프레임당 3~4 ms다. `REXBOX_D3D8_TRACE`는 켜져 있지 않았다.

## 2. 원인

`TraceOnce(key, text)`는 `std::string` 두 개를 받으므로 호출하는 쪽이 `TraceOnce`가 추적 여부를 보기 **전에** 문장을 만든다. D3D8의 10곳 중 3곳만 `D3d8TraceEnabled()`로 감쌌다. 그중 객체마다 불리는 두 곳:

* `D3DDevice_SetModelView`: 호출마다 모델뷰 × 투영 × 뷰포트를 곱해 composite와 비교하고, 실수 다섯 개를 `%g`로 쓴다. 비교 결과는 추적에만 쓴다.
* `D3DDevice_SetVertexShaderInput`: 호출마다 문자열 두 개를 만든다.

텍스처 업로드(`Upload`)의 추적도 감싸지 않았다(업로드마다, 덜 잦음).

## 3. 설계

* 세 곳을 `if (D3d8TraceEnabled())`로 감싼다. `SetModelView`의 곱셈과 비교도 그 안으로 옮긴다(동작은 바뀌지 않는다: 결과는 추적 문장에만 쓰인다).
* `TraceOnce`의 주석에 "인자는 검사 전에 만들어지므로 호출·그리기마다 지나는 곳은 먼저 `D3d8TraceEnabled()`를 본다"를 적는다.
* 프로그램 생성, 리소스 생성, 오류 경로의 `TraceOnce`/`ReportOnce`는 드물게 지나므로 그대로 둔다. 커널 HLE의 `TraceOnce`(kernel_state.cpp)는 원시 장치 경로라 이번 범위가 아니다.

## 4. 검증

* 소비자의 같은 입력 스크립트로 운전 구간(present 2500~3200)의 `REXBOX_D3D8_PROFILE`(fps, CPU, `other`)와 `REXBOX_SAMPLE`의 ucrtbase 비율을 비교한다.
* `REXBOX_D3D8_TRACE=1`에서 "SetModelView: composite row 3" 줄이 여전히 한 번 나온다.
* Win32 x86 Release 빌드, CTest.

---

# Design: no D3D8 trace text while tracing is off (issue #22)

## 1. Requirement

Consumer reexec/reCT3#17: sampling the presenting thread while Crazy Taxi 3 drives (about 810 draws per frame, 42 ms) puts ucrtbase's printf internals (the format-parsing state machine, float digit generation and the `fegetround` it calls, `_aulldiv`) and string allocations at about 8–9% of the samples, 3–4 ms per frame, with `REXBOX_D3D8_TRACE` not set.

## 2. Cause

`TraceOnce(key, text)` takes two `std::string`s, so the caller builds the text **before** `TraceOnce` looks at whether tracing is on. Only 3 of the 10 D3D8 sites are wrapped in `D3d8TraceEnabled()`. Two of the others run per object:

* `D3DDevice_SetModelView` multiplies model view × projection × viewport, compares the product with the composite and writes five floats with `%g` on every call; the comparison feeds only the trace.
* `D3DDevice_SetVertexShaderInput` builds two strings on every call.

The trace in texture upload (`Upload`) is unwrapped too (once per upload, less often).

## 3. Design

* Wrap the three sites in `if (D3d8TraceEnabled())`, with `SetModelView`'s product and comparison moved inside (behaviour is unchanged: the result only went into the trace text).
* `TraceOnce`'s comment says the arguments are built before the check, so a site reached on every call or draw tests `D3d8TraceEnabled()` first.
* `TraceOnce`/`ReportOnce` at program creation, resource creation and error paths are rare and stay. The kernel HLE's own `TraceOnce` (kernel_state.cpp) is on raw-device paths and out of scope.

## 4. Verification

* In the consumer, with the same input script, compare `REXBOX_D3D8_PROFILE` (fps, CPU, `other`) and the ucrtbase share in `REXBOX_SAMPLE` over the driving span (presents 2500–3200).
* With `REXBOX_D3D8_TRACE=1` the "SetModelView: composite row 3" line still appears once.
* Win32 x86 Release build, CTest.
