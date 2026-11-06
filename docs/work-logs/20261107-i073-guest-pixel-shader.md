# 작업 로그: 타이틀의 라이브러리가 만든 픽셀 셰이더 (issue #73)

설계: [20261107-i073](../design/20261107-i073-guest-pixel-shader.md) · 소비자: reexec/reEXCEED#9. 작업 지시는 설계 §2로 갈음한다.

## 한 일

1. 추적:
   * 사용자가 잠긴 곡이 보이는 곡 선택 화면에서 F9로 프레임 캡처(`capture_5882`)를 받았다.
   * 큰 미리보기(`#29`)는 화면의 검은 창 자리에 정확히 그려지고 있었다. 이 호출과 잠긴 칸(`#16`)만 `ps 04200360`을 썼고, 정상 칸은 `ps 0`이었다.
   * xemu 화면에서는 잠긴 곡이 회색조 그림과 오른쪽 위 자물쇠로 나왔다. 그래서 회색조 픽셀 셰이더가 빠진 것으로 좁혀졌다.
   * 그리기 경로의 `FindPixelShader`가 이 핸들을 알지 못해 호출을 건너뛰고 있었다.
2. `src/hle/d3d8/d3d8_shader.cpp`: `FindPixelShader`가 게스트 객체 핸들(`[handle + 8]` → 정의)을 읽는다.

## 검증

* Win32 x86 Release(reEXCEED 트리): 오류 0, 경고 0.
* reEXCEED: 잠긴 곡(FOREVER LOVE)의 칸 그림과 미리보기가 xemu처럼 회색조로 나온다(사용자 확인, 2026-11-07).

---

# Work log: a pixel shader the title's library created (issue #73)

Design: [20261107-i073](../design/20261107-i073-guest-pixel-shader.md) · Consumer: reexec/reEXCEED#9. The design's section 2 stands in for a work order.

Done:
1. Tracing:
   - the user took a frame capture with F9 (`capture_5882`) on a song select that showed locked songs;
   - the big preview (`#29`) was drawn exactly where the black window was, and only it and the locked tile (`#16`) used `ps 04200360`, while the normal tiles used `ps 0`;
   - xemu shows locked songs as grey pictures with a lock at the top right, which narrowed the problem to the missing grey pixel shader;
   - `FindPixelShader` on the draw path did not know that handle and skipped the call.
2. `src/hle/d3d8/d3d8_shader.cpp`: `FindPixelShader` reads a guest object handle (`[handle + 8]` → the definition).

Verification:
- Win32 x86 Release (in the reEXCEED tree): 0 errors and 0 warnings.
- In reEXCEED the locked song's (FOREVER LOVE) tile picture and preview come out grey as in xemu (the user's check, 2026-11-07).
