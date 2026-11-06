# 설계: 오디오 출력 WAV 덤프와 REXBOX_DSOUND_TRACE의 신호 레벨 (issue #72)

상태: **구현됨**. 작업 지시: [20261106-i072](../work-orders/20261106-i072-audio-dump-and-levels.md) · 작업 로그: [20261106-i072](../work-logs/20261106-i072-audio-dump-and-levels.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

Pump It Up: Exceed에서 소리가 들리지 않는다(reEXCEED#9). #71의 통계로 보면 음악 스트림에는 패킷이 제 속도로 들어오고 믹서가 그것을 소비하며, 효과음 버퍼(Xbox ADPCM)도 `Play`된다. 남은 질문은 셋이다.

* 샘플 자체가 무음인가(타이틀 디코더가 0을 내는 경우)?
* 볼륨이나 headroom이 음을 죽이는가?
* 섞인 출력이 0이 아닌 레벨로 호스트 장치에 가는가?

진단이 목적이며 **동작은 바꾸지 않는다**.

## 2. 설계

### 2.1 `REXBOX_AUDIO_DUMP=<파일>`

* 믹서 출력(`RenderOutput`이 clamp한 뒤의 48 kHz 스테레오)을 16비트 PCM WAV로 쓴다.
* `REXBOX_AUDIO=none`(null 장치)에서도 `RenderOutput`이 불리면 쓴다. 불리지 않는다면 작업 로그에 그렇다고 적는다.
* 파일은 출력을 시작할 때(`StartOutputLocked`) 연다. RIFF/data 크기는 약 1초마다 갱신해(seek 후 다시 씀), 프로세스가 강제 종료돼도 그때까지의 내용은 읽을 수 있게 한다.
* 쓰기는 오디오 스레드에서 한다. 실패하면 한 번만 보고하고 덤프를 끈다.

### 2.2 신호 레벨(`REXBOX_DSOUND_TRACE`의 주기 보고)

* 스트림: 마지막 보고 뒤 섞은 샘플의 최대 절댓값을 둘 둔다. 게인 전(`peak_raw`, 패킷 데이터)과 게인 뒤(`peak_out`)다. 주기 보고 줄 끝에 `peak raw X.XXX out X.XXX, volume V, headroom H`를 덧붙이고, 보고한 뒤 0으로 되돌린다.
* 출력 전체: `RenderOutput`에서 clamp 전 최대 절댓값과, 그 구간에 재생 중이던 버퍼 수의 최댓값을 모은다. 주기 보고마다 한 줄을 쓴다.
  ```
  [dsound] output: peak X.XXX, buffers playing N (max), streams N
  ```
  그다음 되돌린다.
* 레벨 수집은 추적이 꺼져 있으면 하지 않는다(덤프는 추적과 상관없이 `REXBOX_AUDIO_DUMP`만 본다).

## 3. 검증

* dsound probe에 검사를 더한다. 진폭 0.5인 16비트 패킷을 섞으면 `peak_raw`가 0.5±0.01이다. 볼륨 −600(약 0.5배)이면 `peak_out`이 대략 그만큼 줄어든다. 테스트용 읽기 함수는 #71의 `StreamStatsForTest`를 넓혀도 된다.
* `REXBOX_AUDIO_DUMP`는 probe에서 짧은 파일을 써서 RIFF 헤더(크기, 48000 Hz, 2 ch, 16 bit)를 확인한다. probe가 `RenderOutput` 경로를 탈 수 없으면, 헤더를 쓰는 함수를 따로 시험한다.
* Win32 x86 Debug·Release 오류 0·경고 0, CTest 통과.

---

# Design: an audio output WAV dump and signal levels in REXBOX_DSOUND_TRACE (issue #72)

Status: **implemented**. Work order: [20261106-i072](../work-orders/20261106-i072-audio-dump-and-levels.md) · Work log: [20261106-i072](../work-logs/20261106-i072-audio-dump-and-levels.md). Consumer: reexec/reEXCEED#9.

**Requirement.** No sound is heard in Pump It Up: Exceed (reEXCEED#9). #71's statistics show the music stream receiving packets at the right rate and the mixer consuming them, and the effect buffers (Xbox ADPCM) being `Play`ed. Three questions remain:
- are the samples themselves silent (a title decoder producing zeros)?
- does a volume or headroom mute a voice?
- does the mixed output reach the host device at a non-zero level?

This is a diagnostic; **behavior does not change**.

**Design.**

`REXBOX_AUDIO_DUMP=<file>`:
- writes the mixer's output (`RenderOutput` after its clamp, 48 kHz stereo) as a 16-bit PCM WAV;
- writes with `REXBOX_AUDIO=none` (the null device) too, whenever `RenderOutput` is called there; if it is not called, the work log says so;
- opens the file when output starts (`StartOutputLocked`) and rewrites the RIFF/data sizes about once a second (seek and rewrite), so a killed process leaves a readable file;
- writes on the audio thread; a failure is reported once and turns the dump off.

Signal levels, in the periodic `REXBOX_DSOUND_TRACE` reports:
- Per stream, two maximum absolute sample values since the last report: before gain (`peak_raw`, the packet data) and after gain (`peak_out`). The periodic line ends with `peak raw X.XXX out X.XXX, volume V, headroom H`, and both reset after each report.
- For the whole output, `RenderOutput` collects the maximum absolute value before the clamp and the maximum number of buffers playing during the interval. Each report adds one line, in the format above, and then resets them.
- Levels are collected only with tracing on. The dump looks only at `REXBOX_AUDIO_DUMP`.

**Verification.**
- The dsound probe gains checks: mixing a 16-bit packet of amplitude 0.5 gives `peak_raw` 0.5 ± 0.01, and volume −600 (about ×0.5) lowers `peak_out` by about that much. #71's `StreamStatsForTest` may be widened for the probe.
- For `REXBOX_AUDIO_DUMP`, the probe writes a short file and checks its RIFF header (sizes, 48000 Hz, 2 ch, 16 bit). If the probe cannot go through `RenderOutput`, the header writer is tested on its own.
- Win32 x86 Debug and Release build with 0 errors and 0 warnings, and CTest passes.
