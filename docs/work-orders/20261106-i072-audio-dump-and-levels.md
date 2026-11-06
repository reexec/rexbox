# 작업 지시: 오디오 출력 WAV 덤프와 신호 레벨 (issue #72)

설계: [20261106-i072](../design/20261106-i072-audio-dump-and-levels.md)

1. `src/hle/dsound/dsound_mixer.cpp`: `REXBOX_AUDIO_DUMP`(열기는 `StartOutputLocked`, 쓰기는 `RenderOutput`, 약 1초마다 헤더 크기 갱신). 출력 peak와 재생 중인 버퍼 수 수집(추적 때만).
2. `src/hle/dsound/dsound_stream.cpp`: 스트림별 `peak_raw`, `peak_out`. 주기 보고 줄에 레벨, 볼륨, headroom을 덧붙이고 `[dsound] output: …` 줄을 쓴다.
3. `tools/dsound_probe/main.cpp`: 설계 §3의 검사.
4. Win32 x86 Debug·Release 빌드(reEXCEED 트리 `build/win32_x86`), `ctest -C Debug`, `ctest -C Release`.
5. 작업 로그 `docs/work-logs/20261106-i072-audio-dump-and-levels.md`(한국어, 그다음 영어)를 쓰고, 설계 문서의 상태를 **구현됨**으로 바꾼다.

---

# Work order: an audio output WAV dump and signal levels (issue #72)

Design: [20261106-i072](../design/20261106-i072-audio-dump-and-levels.md)

1. `src/hle/dsound/dsound_mixer.cpp`: `REXBOX_AUDIO_DUMP` (opened in `StartOutputLocked`, written in `RenderOutput`, header sizes refreshed about once a second); output peak and playing-buffer count collection (tracing only).
2. `src/hle/dsound/dsound_stream.cpp`: per-stream `peak_raw` and `peak_out`; the levels, volume and headroom appended to the periodic line, and the `[dsound] output: …` line.
3. `tools/dsound_probe/main.cpp`: the checks of the design's section 3.
4. Build Win32 x86 Debug and Release (the reEXCEED tree's `build/win32_x86`) and run `ctest -C Debug` and `ctest -C Release`.
5. Write the work log `docs/work-logs/20261106-i072-audio-dump-and-levels.md` (Korean, then English) and set the design's status to **implemented**.
