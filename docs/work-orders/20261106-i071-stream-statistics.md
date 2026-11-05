# 작업 지시: REXBOX_DSOUND_TRACE의 스트림별 패킷 통계 (issue #71)

설계: [20261106-i071](../design/20261106-i071-stream-statistics.md)

1. `src/hle/dsound/dsound_stream.cpp`: `Stream`에 설계 §2의 통계 필드를 넣고, 표의 위치에서 센다.
2. 같은 파일: 주기 보고(`MixStreamsLocked`, 5초)와 처음 8번의 개별 호출 기록(`State().trace`일 때만).
3. probe용 통계 읽기 함수를 내부 헤더(`dsound_internal.h` 또는 `dsound_stream.h`)에 선언하고, `tools/dsound_probe`에 설계 §3의 검사를 더한다.
4. Win32 x86 Debug·Release 빌드(reEXCEED 트리 `build/win32_x86`), `ctest -C Debug`, `ctest -C Release`.
5. 작업 로그 `docs/work-logs/20261106-i071-stream-statistics.md`(한국어, 그다음 영어)를 쓴다.

---

# Work order: per-stream packet statistics in REXBOX_DSOUND_TRACE (issue #71)

Design: [20261106-i071](../design/20261106-i071-stream-statistics.md)

1. `src/hle/dsound/dsound_stream.cpp`: add the counters of the design's section 2 to `Stream` and count them where the table says.
2. Same file: the periodic report (`MixStreamsLocked`, five seconds) and the first-eight-calls log (only with `State().trace`).
3. Declare a statistics reader for the probe in an internal header (`dsound_internal.h` or `dsound_stream.h`) and add the design's section 3 checks to `tools/dsound_probe`.
4. Build Win32 x86 Debug and Release (the reEXCEED tree's `build/win32_x86`) and run `ctest -C Debug` and `ctest -C Release`.
5. Write the work log `docs/work-logs/20261106-i071-stream-statistics.md` (Korean, then English).
