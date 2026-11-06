# 작업 로그: REXBOX_DSOUND_TRACE의 스트림별 패킷 통계 (issue #71)

설계: [20261106-i071](../design/20261106-i071-stream-statistics.md) · 작업 지시: [20261106-i071](../work-orders/20261106-i071-stream-statistics.md) · 소비자: reexec/reEXCEED#9

## 한 일

* `src/hle/dsound/dsound_stream.cpp`: `Stream`에 패킷 통계 필드(`submitted`, `submitted_bytes`, `mixed_out`, `completed`, `starved_mixes`, `paused_mixes`, `flushes`, `discontinuities`, `pause_calls`, `last_delivery`)를 더하고 `DSStream_Process`, `MixStream`, `DeliverCompletions`, `DSStream_Flush`/`IDirectSoundStream_FlushEx`, `DSStream_Discontinuity`, `IDirectSoundStream_Pause`에서 센다.
* 같은 파일: `State().trace`가 켜졌을 때 스트림마다 처음 8번의 `Process`, `Pause`, `Flush`/`FlushEx`, `Discontinuity` 호출을 기록하고, `MixStreamsLocked`에서 5초마다 스트림별 주기 보고 한 줄을 쓴다.
* `src/hle/dsound/dsound_internal.h`, `tools/CMakeLists.txt`, `tools/dsound_probe/main.cpp`: probe 전용 내부 진단 함수 `StreamStatsForTest`를 선언·정의하고, `rexbox_dsound_probe`에 스트림 패킷 제출·믹싱·완료 전달·기아(starved)·일시 정지·불연속·비우기 통계 검사를 더했다(73 → 81 checks).

## 검증

* Win32 x86 Release·Debug 빌드(reEXCEED 트리 `build/win32_x86`): 오류 0, 경고 0.
* `rexbox_dsound_probe`: 81 checks, 0 failures.
* CTest(`ctest -C Release`, `ctest -C Debug`): 7/7 통과.

---

# Work log: per-stream packet statistics in REXBOX_DSOUND_TRACE (issue #71)

Design: [20261106-i071](../design/20261106-i071-stream-statistics.md) · Work order: [20261106-i071](../work-orders/20261106-i071-stream-statistics.md) · Consumer: reexec/reEXCEED#9

Done:
- `src/hle/dsound/dsound_stream.cpp`: added the packet statistics fields (`submitted`, `submitted_bytes`, `mixed_out`, `completed`, `starved_mixes`, `paused_mixes`, `flushes`, `discontinuities`, `pause_calls`, `last_delivery`) to `Stream` and counted them in `DSStream_Process`, `MixStream`, `DeliverCompletions`, `DSStream_Flush`/`IDirectSoundStream_FlushEx`, `DSStream_Discontinuity` and `IDirectSoundStream_Pause`.
- Same file: with `State().trace` enabled, logged the first eight `Process`, `Pause`, `Flush`/`FlushEx` and `Discontinuity` calls per stream and emitted the five-second periodic per-stream report line in `MixStreamsLocked`.
- `src/hle/dsound/dsound_internal.h`, `tools/CMakeLists.txt`, `tools/dsound_probe/main.cpp`: declared and defined the internal diagnostic reader `StreamStatsForTest` for the probe, and added checks for stream packet submission, mixing, completion delivery, starved mixes, paused mixes, discontinuities and flushes to `rexbox_dsound_probe` (73 → 81 checks).

Verification:
- Win32 x86 Release and Debug builds (in the reEXCEED tree's `build/win32_x86`): 0 errors, 0 warnings.
- `rexbox_dsound_probe`: 81 checks, 0 failures.
- CTest (`ctest -C Release` and `ctest -C Debug`): 7/7 passed.
