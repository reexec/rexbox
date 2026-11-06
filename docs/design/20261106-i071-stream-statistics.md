# 설계: REXBOX_DSOUND_TRACE의 스트림별 패킷 통계 (issue #71)

상태: **구현됨**. 작업 지시: [20261106-i071](../work-orders/20261106-i071-stream-statistics.md) · 작업 로그: [20261106-i071](../work-logs/20261106-i071-stream-statistics.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

`REXBOX_DSOUND_TRACE`는 스트림을 만들 때 한 줄만 남긴다. 소리가 나지 않는 스트림이 다음 가운데 어느 경우인지 구별할 수 없다.

* 패킷이 들어오지 않는다.
* 들어왔는데 섞이지 않는다(일시 정지, 형식 무효).
* 섞였는데 완료 통지가 전달되지 않는다(완료는 게스트 스레드가 `DirectSoundDoWork`나 스트림 메서드를 부를 때만 전달된다).

Pump It Up: Exceed는 음악을 스트림 하나로 낸다(타이틀의 WMA 디코더가 채움, 2 ch 44100 Hz 16 bit, 패킷 43개, 콜백). 그런데 음악이 나지 않을 때가 있다(reEXCEED#9). 진단이 목적이며, **동작은 바꾸지 않는다**.

## 2. 설계

* `Stream`에 통계를 둔다(`State().mutex`로 보호, 추적이 꺼져 있어도 센다. 정수 증가만이다).

| 필드 | 늘어나는 곳 |
|---|---|
| `submitted`, `submitted_bytes` | `DSStream_Process`가 패킷을 큐에 넣을 때 |
| `mixed_out` | `MixStream`이 패킷을 다 재생해 `done`으로 만들 때 |
| `completed` | `DeliverCompletions`가 그 스트림의 패킷을 완료할 때 |
| `starved_mixes` | `MixStream`이 (일시 정지 아님, 형식 유효) 재생할 패킷이 없어 출력 일부를 비운 호출 수 |
| `paused_mixes` | `MixStream`이 일시 정지로 아무것도 하지 않은 호출 수 |
| `flushes`, `discontinuities`, `pause_calls` | `Flush`/`FlushEx`, `Discontinuity`, `IDirectSoundStream_Pause` |
| `last_delivery` | `DeliverCompletions`가 마지막으로 그 스트림의 패킷을 완료한 시각(`steady_clock`) |

* 보고(`State().trace`가 켜졌을 때만):
  * **주기 보고**: `MixStreamsLocked`(호스트 오디오 스레드, 게스트가 멈춰도 돈다)에서 마지막 보고 뒤 5초가 지났으면, 스트림마다 한 줄을 쓴다. 형식은 아래와 같다.
    ```
    [dsound] stream XXXXXXXX: submitted N (B bytes), mixed N, completed N, queued N (D done), starved N, paused-mixes N, flushes N, discontinuities N, pauses N (now paused|playing), last delivery X.X s ago|never
    ```
    `queued`는 큐 길이, `D done`은 그 가운데 재생이 끝났는데 아직 완료 통지가 나가지 않은 수다.
  * **처음 몇 번의 개별 호출**: 스트림마다 `Process`, `Pause`, `Flush`/`FlushEx`, `Discontinuity`를 각각 처음 8번 기록한다(`Process`는 버퍼, 바이트 수, 이벤트/컨텍스트, `Pause`는 인자).
  * mutex를 쥔 채 `fprintf`하는 것은 진단 때만 생기므로 허용한다.
* 추적이 꺼져 있으면 출력이 없다. 통계 외의 동작은 바꾸지 않는다.

## 3. 검증

* dsound probe에 검사를 더한다: 형식이 유효한 스트림에 패킷 2개를 `Process`하고 섞은 뒤(`MixStreamsLocked`), 완료를 전달하면 `submitted` 2, `mixed_out` 2, `completed` 2가 된다. 패킷 없이 섞으면 `starved_mixes`가 늘어난다. 통계를 읽는 진단 API(`StreamStatsForTest(GuestAddress)` 같은 내부 함수)는 probe에서만 쓴다.
* Win32 x86 Debug·Release 오류 0·경고 0, CTest 통과.
* reEXCEED에서 `REXBOX_DSOUND_TRACE=1`로 본게임까지 실행해, 음악 스트림의 주기 보고가 나오는지 본다.

---

# Design: per-stream packet statistics in REXBOX_DSOUND_TRACE (issue #71)

Status: **implemented**. Work order: [20261106-i071](../work-orders/20261106-i071-stream-statistics.md) · Work log: [20261106-i071](../work-logs/20261106-i071-stream-statistics.md). Consumer: reexec/reEXCEED#9.

**Requirement.** `REXBOX_DSOUND_TRACE` reports a stream only when it is created. A silent stream cannot be told apart among these cases:
- no packets arrive;
- packets arrive but are not mixed (paused, invalid format);
- packets are mixed but their completions are not delivered (completions go out only when a guest thread calls `DirectSoundDoWork` or a stream method).

Pump It Up: Exceed plays its music through one stream, fed by the title's WMA decoder (2 ch, 44100 Hz, 16 bit, 43 packets, a callback), and the music sometimes does not play (reEXCEED#9). This is a diagnostic; **behavior does not change**.

**Design.** `Stream` gains counters, guarded by `State().mutex` and counted even with tracing off (plain increments), as in the table above:
- `submitted` / `submitted_bytes` in `DSStream_Process`;
- `mixed_out` when `MixStream` plays a packet out;
- `completed` in `DeliverCompletions`;
- `starved_mixes`: `MixStream` calls that, not paused and with a valid format, left output empty for want of a packet;
- `paused_mixes`;
- `flushes`, `discontinuities` and `pause_calls`;
- `last_delivery`, a `steady_clock` time.

Reports go out only with `State().trace`:
- **Periodic**: in `MixStreamsLocked` (the host audio thread, which runs even when the guest stalls), once five seconds have passed since the last report, one line per stream in the format above. `queued` is the queue length; `D done` is how many of those finished playing but have not been completed yet.
- **First calls**: the first eight `Process` calls per stream, and likewise `Pause`, `Flush`/`FlushEx` and `Discontinuity`, each logged individually (`Process` with buffer, bytes and event/context; `Pause` with its argument).
- `fprintf` with the mutex held happens only while diagnosing and is accepted.

With tracing off nothing is printed, and nothing but the counters changes.

**Verification.**
- The dsound probe gains checks. On a stream with a valid format, two packets are `Process`ed, mixed (`MixStreamsLocked`) and their completions delivered: that gives `submitted` 2, `mixed_out` 2 and `completed` 2. A mix with no packet raises `starved_mixes`. The probe reads the counters through an internal diagnostic function such as `StreamStatsForTest(GuestAddress)`, used by the probe only.
- Win32 x86 Debug and Release build with 0 errors and 0 warnings, and CTest passes.
- In reEXCEED, `REXBOX_DSOUND_TRACE=1` into gameplay must show the music stream's periodic reports.
