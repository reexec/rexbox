# 설계: 느린 프레임 보고에 present하는 스레드의 대기·잠금·파일 읽기 (issue #80)

상태: 설계. 소비자: reexec/reCT3#41. 이전: [i078 VAO 비용](20261107-i078-vao-cost.md).

## 1. 요구사항

v0.0.28에서 Crazy Taxi 3의 밀집 주행 구간은 600프레임 창 평균 59 fps가 됐지만, 프레임은 아직 고르지 않다. West Coast 주행을 `REXBOX_D3D8_SLOW_FRAME=20`으로 돌렸다(약 9800프레임, 타이틀 화면 뒤 느린 프레임 115개). 느린 프레임을 가장 큰 항목으로 나누면 다음과 같다.

| 가장 큰 항목 | 프레임 | 길이 | 비고 |
|---|---|---|---|
| HLE 단계 밖("other", 타이틀 자신의 시간) | 24 | 중앙값 56 ms | 주행 중 100 ms(프레임 3602, 4202, draw 약 790), 장면 전환 때 342 ms까지 |
| draw 단계 | 40 | 20~33 ms | 구역에 들어설 때. 한 프레임에 텍스처 112개, 프로그램 25개까지 |
| present | 37 | 20~40 ms | 스왑이 막힘 |
| 수직 귀선 대기 | 14 | | 틱을 넘겨 다음 틱까지 기다림 |

"other" 시간에 타이틀 스레드가 무엇을 했는지 지금 보고로는 알 수 없다. 게스트 코드를 돌렸는지, 기다렸는지(커널 대기, 임계 구역, reCT3의 CRI ADXM 잠금 같은 rexbox 호스트 게스트 잠금, 파일 읽기) 구분되지 않는다. 보고의 `cpu`는 OS 스레드 시간의 틱(Windows 15.6 ms) 단위라 이것도 가르지 못한다.

## 2. 설계

### 2.1 runtime: 스레드별 대기 합계 (`thread_activity`)

* 모든 대기는 이미 `NoteWaitBegin`/`NoteWaitEnd`(`ScopedWait`, 커널의 `WaitNote`)를 지난다.
  * 커널 대기: `WaitAndDeliver`, `KeDelayExecutionThread`.
  * 막힌 `RtlEnterCriticalSection`.
  * 막힌 `rexbox_guest_lock_enter`(`guest_lock.cpp`의 `Acquire`).
  * D3D8의 수직 귀선·present 간격 대기.
  * 그 밖의 스레드 시작·정지 대기.
* 여기에 스레드별 합계를 더한다. 켜져 있을 때만 센다.
  * `void EnableWaitTotals()`: 켠다. 한 번 켜면 끄지 않는다.
  * `NoteWaitBegin`은 바깥쪽 대기(중첩 깊이 0)일 때 시작 시각과 `what`을 TLS에 적는다. `NoteWaitEnd`가 길이를 합계에 더한다. 중첩된 대기는 바깥 대기 하나로 센다.
  * 합계는 대기 시간(ns), 횟수, 가장 긴 대기의 길이와 `what`이다.
  * `what`이 `"D3D8 "`으로 시작하는 대기(수직 귀선, present 간격)는 프로파일이 `wait` 단계로 이미 재므로 합계에서 뺀다.
* 파일 읽기: `void NoteFileRead(uint64_t bytes, uint64_t ns)`.
  * 커널의 `NtReadFile`(과 `NtReadFileScatter`)이 읽기 전후를 재서 부른다. 켜져 있을 때만 시계를 읽는다.
  * 합계는 읽기 수, 바이트, 시간이다.
  * 읽기가 커널 대기 안에서 일어나지는 않으므로, 대기 합계와 겹치지 않는다.
* `WaitTotals TakeWaitTotals()`: 부른 스레드의 합계를 돌려주고 0으로 되돌린다.
* 시계는 `std::chrono::steady_clock`이다. 켜지지 않았으면 원자 플래그 하나만 읽는다.

### 2.2 d3d8 프로파일

* `REXBOX_D3D8_SLOW_FRAME`이 켜져 있으면 처음에 `EnableWaitTotals()`를 부른다.
* 프레임이 끝날 때마다(present하는 스레드의 `EndFrame`) `TakeWaitTotals()`로 그 프레임의 합계를 받는다. 느린 프레임 줄에는 다음을 붙인다.
  ```
  | waits 78.0 ms (12, longest 70.2 RtlEnterCriticalSection), reads 3 (256 KiB, 2.1 ms)
  ```
* 느린 프레임이 아니면 받기만 하고 버린다. 합계가 다음 프레임으로 넘어가지 않게 하기 위해서다.

## 3. 비용

* 기본(변수 없음)에서는 대기마다 플래그 하나를 읽는다. 대기는 이미 OS 대기를 수반하므로 무시할 만하다.
* 켜면 대기와 읽기마다 시계를 두 번 읽는다.

## 4. 검증

* kernel·d3d8 probe, CTest.
* reCT3 West Coast 주행(`REXBOX_D3D8_SLOW_FRAME=20`)으로 "other" 프레임의 대기·읽기를 읽는다. 결과로 다음 issue(잠금 경쟁, 파일 읽기, 업로드, 스왑)를 정한다.

---

# Design: the presenting thread's waits, locks and file reads in the slow-frame report (issue #80)

Status: design. Consumer: reexec/reCT3#41. Earlier: [i078 VAO cost](20261107-i078-vao-cost.md).

## 1. Requirement

With v0.0.28 Crazy Taxi 3's dense driving stretches average 59 fps per 600-frame window, but frames are still not steady. A West Coast drive with `REXBOX_D3D8_SLOW_FRAME=20` (about 9800 frames, 115 slow frames after the title screen) splits the slow frames by their largest item:

| Largest item | Frames | Length | Notes |
|---|---|---|---|
| Outside the HLE phases ("other", the title's own time) | 24 | median 56 ms | 100 ms during driving (frames 3602, 4202, about 790 draws), up to 342 ms at scene changes |
| Draw phases | 40 | 20–33 ms | entering an area; up to 112 textures and 25 programs in one frame |
| Present | 37 | 20–40 ms | the swap blocks |
| Vertical-blank wait | 14 | | overran a tick and waited for the next |

The report cannot say what the title's thread did in the "other" time: whether it ran guest code or waited (kernel waits, critical sections, rexbox's host guest locks such as reCT3's CRI ADXM lock, file reads). Its `cpu` figure is only as fine as the OS thread-time tick (15.6 ms on Windows), so it cannot tell them apart either.

## 2. Design

### 2.1 runtime: per-thread wait totals (`thread_activity`)

* Every wait already passes through `NoteWaitBegin`/`NoteWaitEnd` (`ScopedWait`, the kernel's `WaitNote`):
  * kernel waits: `WaitAndDeliver`, `KeDelayExecutionThread`;
  * a blocking `RtlEnterCriticalSection`;
  * a blocking `rexbox_guest_lock_enter` (`Acquire` in `guest_lock.cpp`);
  * D3D8's vertical-blank and present-interval waits;
  * the other thread start and suspend waits.
* Per-thread totals are added there, counted only when enabled.
  * `void EnableWaitTotals()` turns them on; once on they stay on.
  * `NoteWaitBegin` records the start time and `what` in TLS for an outermost wait (nesting depth 0), and `NoteWaitEnd` adds its length to the totals. A nested wait counts as the one outer wait.
  * The totals are the wait time (ns), the number of waits, and the longest wait's length and `what`.
  * Waits whose `what` starts with `"D3D8 "` (the vertical blank, the present interval) are left out, since the profile already measures them as its `wait` phase.
* File reads: `void NoteFileRead(uint64_t bytes, uint64_t ns)`.
  * The kernel's `NtReadFile` (and `NtReadFileScatter`) times the read and calls it, reading the clock only when enabled.
  * The totals are the number of reads, bytes and time.
  * A read does not happen inside a kernel wait, so it does not overlap the wait totals.
* `WaitTotals TakeWaitTotals()` returns the calling thread's totals and resets them to 0.
* The clock is `std::chrono::steady_clock`. When not enabled, only one atomic flag is read.

### 2.2 d3d8 profile

* With `REXBOX_D3D8_SLOW_FRAME` set, the profile calls `EnableWaitTotals()` first.
* At each frame's end (`EndFrame` on the presenting thread) it takes that frame's totals with `TakeWaitTotals()`, and a slow-frame line gains:
  ```
  | waits 78.0 ms (12, longest 70.2 RtlEnterCriticalSection), reads 3 (256 KiB, 2.1 ms)
  ```
* For a frame that is not slow the totals are taken and dropped, so they never carry over to the next frame.

## 3. Cost

* By default (no variable) each wait reads one flag. A wait already involves an OS wait, so this is negligible.
* When on, each wait and read gains two clock reads.

## 4. Verification

* The kernel and d3d8 probes, CTest.
* A reCT3 West Coast drive (`REXBOX_D3D8_SLOW_FRAME=20`), reading the waits and reads of the "other" frames. The results decide the next issue (lock contention, file reads, uploads, the swap).
