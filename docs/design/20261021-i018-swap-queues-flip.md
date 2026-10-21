# 설계: Swap은 플립을 예약하고 돌아온다 (issue #18)

## 1. 요구사항

소비자 reexec/reCT3#13: Crazy Taxi 3는 프레임당 CPU 0.47 ms인 타이틀 화면에서도 29.5 fps에 묶이고, `REXBOX_D3D8_PROFILE`의 `wait`가 프레임당 32 ms(vblank 두 개)다. 타이틀의 로직은 렌더 프레임마다 1/60초씩 진행하므로(1200프레임에 게임 시간 20초) 30 fps면 게임이 절반 속도로 돈다.

타이틀은 프레임 끝에서 `D3DDevice_Swap(0)`을 부르고 곧바로 `D3DDevice_BlockUntilVerticalBlank()`를 부른다(소비자 `000A7085`~`000A708C`). 표시 간격은 1(`CreateDevice`의 `PresentationInterval`)이다.

## 2. 현재 구조와 원인

* `D3DDevice_Swap`은 `WaitForPresentInterval(interval)`로 **앞 present 뒤 `interval`번째 vblank까지 잠든 다음** 그림을 보낸다. 늦었으면 다음 vblank까지 잔다. 그래서 `Swap`은 언제나 vblank 경계에서 돌아온다.
* `BlockUntilVerticalBlank`는 다음 vblank까지 잔다. `Swap`이 vblank 직후에 돌아오므로 다시 한 주기 전부를 기다린다.
* Xbox에서 `Swap`은 플립을 푸시 버퍼에 넣고 돌아온다. GPU가 vblank에서 플립하고, CPU는 앞 플립이 아직 끝나지 않았을 때만 기다린다. 그래서 하드웨어에서는 `Swap` 다음의 vblank 대기가 프레임의 유일한 대기이고 60 fps가 된다.
* 호스트 vsync 정렬: `PresentRenderTarget`의 호스트 swap이 1 ms 이상 막혔으면 그 반환 시각 H를 호스트 blank로 보고, `AlignVerticalBlank(H)`가 가상 vblank를 H보다 3 ms **앞**(`kAlignLead`)에 둔다. 이 위상은 `Swap`이 가상 vblank에서 깨어나 3 ms 안에 호스트 blank에 맞추는 지금의 흐름을 위한 것이다. `Swap`이 기다리지 않게 되면 타이틀은 H 직전의 vblank에서 프레임을 시작해 3 ms 안에 끝내야 그 호스트 blank에 맞출 수 있다.

## 3. 설계

**플립 예약.** `WaitForPresentInterval`을 `QueuePresent(interval)`로 바꾼다.

* 앞 present가 예약한 vblank(`g_last_present`)가 아직 오지 않았으면 그때까지 잔다(플립 하나만 대기열에 둔다).
* 이 present의 vblank는 `앞 예약 + interval`이고, 이미 지났으면 다음 vblank다. 이를 `g_last_present`에 적고 돌아온다.
* `interval`이 0이거나 `IMMEDIATE`면 지금처럼 기다리지 않고 현재 vblank를 적는다.
* 그림은 지금처럼 `Swap` 안에서 곧바로 호스트로 보낸다(호스트 vsync가 있으면 호스트가 다음 blank에 보인다).

CT3의 흐름: vblank k에서 깨어나 프레임을 그리고, `Swap`은 예약 k가 지났으므로 기다리지 않고 k+1을 예약한다. `BlockUntilVerticalBlank`가 k+1까지 자고, 다음 프레임의 `Swap`은 예약 k+1이 지났으므로 다시 기다리지 않는다. 프레임당 vblank 하나다. 스스로 기다리지 않고 `Swap`만 부르는 타이틀은 두 번째 `Swap`이 앞 예약까지 자므로 지금처럼 vblank 하나에 프레임 하나다.

**정렬 위상.** `AlignVerticalBlank(H)`는 가상 vblank가 호스트 blank보다 `kAlignLag`(1 ms) **뒤**에 오게 한다. 현재 vblank 번호 n(`CountAt(H)`)은 그대로이고 n+1이 H + 1 ms에 온다. 타이틀은 호스트 blank 직후에 프레임을 시작하므로, 다음 호스트 blank까지 주기에서 1 ms와 present 비용을 뺀 시간을 쓸 수 있다. `Swap`만 부르는 타이틀은 작업이 1 ms보다 짧을 때만 예약 vblank까지 잠깐 자고 곧바로 보내므로 같은 호스트 blank에 맞춘다.

바꾸지 않는 것: 가상 클록의 속도(`SetVerticalBlankRate`, 대기 끄기의 10 kHz), `WaitForVerticalBlank`, `RasterPosition`, 정렬의 조건(호스트 swap이 1 ms 이상 막혔고 모니터가 게스트 주사율에서 1 Hz 안).

## 4. 검증

* 소비자 실행(창, vsync): 타이틀 화면의 fps(29.5 → 60 기대)와 `wait`.
* 소비자 운전 화면: game time의 감소 속도와 fps. 프레임당 게임 시간은 1/60초 그대로이고 fps가 오른 만큼 빨라져야 한다.
* 숨긴 창(`REXBOX_DISPLAY=hidden`)과 vsync 끄기(`REXBOX_VSYNC=off`)에서도 타이틀 화면 60 fps.
* CTest(probe 전부).

---

# Design: Swap queues the flip and returns (issue #18)

## 1. Requirement

Consumer reexec/reCT3#13: Crazy Taxi 3 is held at 29.5 fps even on its title screen (0.47 ms of CPU per frame), with `REXBOX_D3D8_PROFILE`'s `wait` at 32 ms (two vblanks) per frame. The title's logic advances 1/60 s per rendered frame (20 s of game time in 1200 frames), so at 30 fps the game runs at half speed.

The title ends each frame with `D3DDevice_Swap(0)` followed at once by `D3DDevice_BlockUntilVerticalBlank()` (consumer `000A7085`–`000A708C`), with presentation interval 1 (`PresentationInterval` at `CreateDevice`).

## 2. Current structure and cause

* `D3DDevice_Swap` **sleeps to the `interval`-th vblank after the previous present** in `WaitForPresentInterval(interval)` and then sends the picture; when late it sleeps to the next vblank. So `Swap` always returns at a vblank boundary.
* `BlockUntilVerticalBlank` sleeps to the next vblank. Since `Swap` has just returned at a vblank, it waits a whole period again.
* On the Xbox `Swap` puts the flip in the push buffer and returns. The GPU flips at the vblank, and the CPU waits only while the previous flip has not happened yet. On the hardware the vblank wait after `Swap` is therefore the frame's only wait, and the title runs at 60 fps.
* Host vsync alignment: when the host swap in `PresentRenderTarget` blocked for 1 ms or more, its return time H is taken as the host's blank, and `AlignVerticalBlank(H)` puts the virtual vblank 3 ms **before** it (`kAlignLead`). That phase serves today's flow, where `Swap` wakes at the virtual vblank and has 3 ms to make the host's blank. Once `Swap` no longer waits, the title would start its frame at the vblank just before H and would have to finish within 3 ms to make that blank.

## 3. Design

**Queued flips.** `WaitForPresentInterval` becomes `QueuePresent(interval)`:

* If the vblank the previous present queued for (`g_last_present`) has not come yet, sleep until it does (one flip in the queue).
* This present's vblank is `previous + interval`, or the next vblank when that has passed. It goes into `g_last_present` and the call returns.
* With `interval` 0 or `IMMEDIATE` there is no wait and the current vblank is recorded, as now.
* The picture still goes to the host at once inside `Swap` (with host vsync, the host shows it at its next blank).

CT3's flow: it wakes at vblank k and draws its frame. `Swap` finds k passed, so it does not wait and queues k+1. `BlockUntilVerticalBlank` sleeps to k+1, and the next frame's `Swap` finds k+1 passed and again does not wait. That is one vblank per frame. A title that only calls `Swap` and never waits itself has its second `Swap` sleep to the queued vblank, so it still gets one frame per vblank as now.

**Alignment phase.** `AlignVerticalBlank(H)` puts the virtual vblank `kAlignLag` (1 ms) **after** the host's blank. The current vblank number n (`CountAt(H)`) is kept and n+1 comes at H + 1 ms. The title starts its frame right after the host's blank and has the period minus 1 ms and the present's cost to make the next one. A title that only calls `Swap` sleeps to the queued vblank only when its work took under 1 ms, then sends at once, and so makes the same host blank.

Unchanged: the clock's rate (`SetVerticalBlankRate`, 10 kHz with the wait off), `WaitForVerticalBlank`, `RasterPosition`, and when to align (a host swap that blocked 1 ms or more, on a monitor within 1 Hz of the guest refresh).

## 4. Verification

* Consumer run (window, vsync): the title screen's fps (29.5, expected to reach 60) and `wait`.
* Consumer driving: how fast the game time counts down, and fps. Game time per frame stays 1/60 s and the game must speed up with the fps.
* Hidden window (`REXBOX_DISPLAY=hidden`) and vsync off (`REXBOX_VSYNC=off`): the title screen at 60 fps too.
* CTest (every probe).
