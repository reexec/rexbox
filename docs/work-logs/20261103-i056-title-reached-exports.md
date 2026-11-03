# 작업 로그: XDK 5849 타이틀이 직접 닿는 HLE export (issue #56)

설계: [20261103-i056](../design/20261103-i056-title-reached-exports.md) · 소비자: reexec/reEXCEED#4. 작업 지시는 설계의 표로 갈음한다(같은 종류의 작은 export 여럿).

## 한 일

* `d3d8_query.cpp`(새 파일): 조회 13개와 할 일 없는·받기만 하는 5개. `D3dState::behavior_flags`.
* `d3d8_shader.cpp`: `WriteVertexConstants`로 묶고 `SetVertexShaderConstantNotInline`, `SetVertexShaderConstant1`, `SetVertexShaderConstantNotInlineFast`, `GetVertexShaderConstant`.
* `dsound_buffer.cpp`: `GetVoiceProperties` 둘과 받기만 하는 다섯.
* `d3d8_device.cpp`: 바인딩된 `D3D_g_Device`를 HLE 디바이스로.

## 검증

* Win32 x86 Debug(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED: 프레임 58장(약 16.7 ms 간격), 게임 스레드 3·4 생성, `XInputOpen(gamepad, port 0)`. 그 뒤 스레드 3이 lift된 코드에서 10초 넘게 HLE를 부르지 않고, 스레드 2가 그 스레드의 임계 구역을 기다린다(reEXCEED 작업 로그 i004에서 이어서 본다). `DrawVerticesUP: vertex format 00000000`, SDL의 Debug 단언(`SDL_PumpEvents`가 메인 스레드가 아닌 곳에서)도 보고된다.

---

# Work log: HLE exports an XDK 5849 title reaches directly (issue #56)

Design: [20261103-i056](../design/20261103-i056-title-reached-exports.md) · Consumer: reexec/reEXCEED#4. The design's table stands in for a work order (many small exports of one kind).

Done: `d3d8_query.cpp` (new) with the thirteen queries and the five that do nothing or accept only, and `D3dState::behavior_flags`; in `d3d8_shader.cpp`, `WriteVertexConstants` shared by `SetVertexShaderConstantNotInline`, `SetVertexShaderConstant1`, `SetVertexShaderConstantNotInlineFast` and `GetVertexShaderConstant`; in `dsound_buffer.cpp`, the two `GetVoiceProperties` and five accepting exports; in `d3d8_device.cpp`, a bound `D3D_g_Device` as the HLE device.

Verification: Win32 x86 Debug (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. reEXCEED presents 58 frames (about 16.7 ms apart), creates game threads 3 and 4 and opens the gamepad on port 0; then thread 3 runs lifted code for over 10 s without an HLE call while thread 2 waits on its critical section (followed up in reEXCEED's work log i004). `DrawVerticesUP: vertex format 00000000` and an SDL Debug assertion (`SDL_PumpEvents` off the main thread) are reported as well.
