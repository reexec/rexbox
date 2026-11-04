# 작업 로그: GL 오류 비우기에 상한 (issue #64)

설계: [20261105-i064](../design/20261105-i064-bounded-gl-error-drain.md) · 소비자: reexec/reEXCEED#7. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* 추적: #62·#63 뒤에도 멈춰서 호스트 스택을 0.4초 간격으로 여섯 번 찍었다. 매번 `glGetError <- GetVideoMemoryInfo`였다.
* `src/hle/d3d8/d3d8_display.cpp`: `DrainGlErrors`(최대 16번), 실패하면 드라이버 조회를 건너뛴다.
* 두 번째 스레드의 GL 작업이 효과가 없는 문제는 [#65](https://github.com/reexec/rexbox/issues/65)로 남겼다.

## 검증

* Win32 x86 Debug·Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED(창 표시, 오디오, 키보드): 타이틀에서 START → SELECT MODE → ARCADE 곡 선택 → 난이도 선택까지, 120초 동안 멈춤 없음.

---

# Work log: a bound on the GL error drain (issue #64)

Design: [20261105-i064](../design/20261105-i064-bounded-gl-error-drain.md) · Consumer: reexec/reEXCEED#7. The design's section 2 stands in for a work order.

Done: tracing — the game still stopped after #62 and #63, and six host-stack snapshots 0.4 s apart all showed `glGetError <- GetVideoMemoryInfo`. In `src/hle/d3d8/d3d8_display.cpp`: `DrainGlErrors` (at most 16 reads), and the driver queries are skipped when it fails. That GL work from a second thread has no effect is left to [#65](https://github.com/reexec/rexbox/issues/65).

Verification: Win32 x86 Debug and Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. In reEXCEED (window shown, audio, keyboard): START at the title → SELECT MODE → the ARCADE song select → difficulty select, no stop in 120 s.
