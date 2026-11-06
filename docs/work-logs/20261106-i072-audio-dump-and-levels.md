# 작업 로그: 오디오 출력 WAV 덤프와 신호 레벨 (issue #72)

설계: [20261106-i072](../design/20261106-i072-audio-dump-and-levels.md) · 작업 지시: [20261106-i072](../work-orders/20261106-i072-audio-dump-and-levels.md) · 소비자: reexec/reEXCEED#9

## 한 일

* `src/hle/dsound/dsound_hle.cpp`, `src/hle/dsound/dsound_mixer.cpp`: `REXBOX_AUDIO_DUMP=<파일>`이 설정되어 있으면 `StartOutputLocked`에서 파일을 열어 44바이트 RIFF/WAVE 헤더(48000 Hz, 2 ch, 16-bit PCM)를 쓰고, `RenderOutput`에서 clamp 뒤의 샘플을 16비트 PCM으로 기록하며 약 1초(192000바이트)마다 RIFF/data 크기를 seek 후 갱신한다. 쓰기 실패 시 한 번만 보고하고 덤프를 끈다.
* null 장치(`REXBOX_AUDIO=none`) 동작 확인: `src/platform/sdl/audio_sdl.cpp`의 `StartAudioOutput`은 `config.null_device`일 때 호스트 오디오 장치를 열지 않고 `PacerMain` 스레드를 띄워 실시간 속도로 `g_render`(`RenderOutput`)를 호출한다. 따라서 `REXBOX_AUDIO=none`에서도 `RenderOutput`이 정상적으로 호출되며 WAV 덤프와 출력 레벨 수집이 그대로 동작한다.
* `src/hle/dsound/dsound_mixer.cpp`, `src/hle/dsound/dsound_stream.cpp`, `src/hle/dsound/dsound_internal.h`: `State().trace`가 켜졌을 때 스트림마다 게인 전·후 최대 절댓값(`peak_raw`, `peak_out`)을 모으고 주기 보고 줄 끝에 `peak raw X.XXX out X.XXX, volume V, headroom H`를 덧붙인 뒤 0으로 되돌린다. 또한 `RenderOutput`에서 clamp 전 최대 절댓값(`output_peak`)과 구간 내 재생 중인 버퍼 수의 최댓값(`max_playing_buffers`)을 모아 주기 보고마다 `[dsound] output: peak X.XXX, buffers playing N (max), streams N` 줄을 출력하고 되돌린다.
* `tools/dsound_probe/main.cpp`: `StreamStatsForTest`에 `peak_raw`, `peak_out`, `volume`, `headroom`을 넓히고, 진폭 0.5 패킷의 `peak_raw`/`peak_out`(볼륨 0 및 −600) 검사와 `WriteWavDumpForTest`를 통한 짧은 WAV 파일의 RIFF 헤더(크기, 48000 Hz, 2 ch, 16 bit) 검사를 더했다(81 → 84 checks).

## 검증

* Win32 x86 Release·Debug 빌드(reEXCEED 트리 `build/win32_x86`): 오류 0, 경고 0.
* `rexbox_dsound_probe`: 84 checks, 0 failures.
* CTest(`ctest -C Release`, `ctest -C Debug`): 7/7 통과.

---

# Work log: an audio output WAV dump and signal levels (issue #72)

Design: [20261106-i072](../design/20261106-i072-audio-dump-and-levels.md) · Work order: [20261106-i072](../work-orders/20261106-i072-audio-dump-and-levels.md) · Consumer: reexec/reEXCEED#9

Done:
- `src/hle/dsound/dsound_hle.cpp`, `src/hle/dsound/dsound_mixer.cpp`: when `REXBOX_AUDIO_DUMP=<file>` is set, `StartOutputLocked` opens the file with a 44-byte RIFF/WAVE header (48000 Hz, 2 ch, 16-bit PCM), and `RenderOutput` writes the post-clamp samples as 16-bit PCM while seeking back about once a second (every 192000 bytes) to refresh the RIFF and `data` sizes; a write failure is reported once and disables the dump.
- Checked how the null audio device (`REXBOX_AUDIO=none`) drives `RenderOutput`: in `src/platform/sdl/audio_sdl.cpp`, `StartAudioOutput` with `config.null_device` skips opening an SDL host audio device and starts the `PacerMain` thread, which calls `g_render` (`RenderOutput`) in 512-frame chunks at real-time pace from `steady_clock`. `RenderOutput` is therefore called under `REXBOX_AUDIO=none` as well, so both `REXBOX_AUDIO_DUMP` and the output peak collection work with the null device.
- `src/hle/dsound/dsound_mixer.cpp`, `src/hle/dsound/dsound_stream.cpp`, `src/hle/dsound/dsound_internal.h`: with `State().trace` enabled, collected per-stream `peak_raw` and `peak_out` in `MixStream`, appended `peak raw X.XXX out X.XXX, volume V, headroom H` to each stream's periodic report line before resetting them, and collected pre-clamp `output_peak` and `max_playing_buffers` in `RenderOutput` to log `[dsound] output: peak X.XXX, buffers playing N (max), streams N` on each periodic report before resetting them.
- `tools/dsound_probe/main.cpp`: widened `StreamStatsForTest` with `peak_raw`, `peak_out`, `volume` and `headroom`, and added checks for a 0.5-amplitude packet at volume 0 and volume −600 plus a short WAV dump RIFF header check (sizes, 48000 Hz, 2 ch, 16 bit) via `WriteWavDumpForTest` (81 → 84 checks).

Verification:
- Win32 x86 Release and Debug builds (in the reEXCEED tree's `build/win32_x86`): 0 errors, 0 warnings.
- `rexbox_dsound_probe`: 84 checks, 0 failures.
- CTest (`ctest -C Release` and `ctest -C Debug`): 7/7 passed.
