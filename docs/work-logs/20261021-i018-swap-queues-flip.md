# 작업 로그: Swap은 플립을 예약하고 돌아온다 — 완료 (issue #18)

작업 지시: [20261021-i018-swap-queues-flip](../work-orders/20261021-i018-swap-queues-flip.md) · 설계: [20261021-i018-swap-queues-flip](../design/20261021-i018-swap-queues-flip.md) · 소비자: reexec/reCT3#13

## 한 일

1. 원인 확인: 소비자 타이틀 화면(프레임당 CPU 0.47 ms)이 29.5 fps, `wait` 32 ms. HLE 추적으로 한 프레임의 호출 순서를 보면 `D3DDevice_Swap` 바로 다음에 타이틀의 `D3DDevice_BlockUntilVerticalBlank`가 온다.
2. `d3d8_vblank.{h,cpp}`: `WaitForPresentInterval`을 `QueuePresent`로 바꿨다. 앞 present의 vblank가 아직 오지 않았을 때만 그때까지 자고, 이 present의 vblank를 예약하고 돌아온다. `kAlignLead`(3 ms 앞)를 `kAlignLag`(1 ms 뒤)로 바꿔 정렬 후 가상 vblank가 호스트 blank 뒤에 온다.
3. `d3d8_device.cpp`: `D3DDevice_Swap`의 두 호출을 `QueuePresent`로.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Release, CTest | 오류 0, 5/5 통과 |
| 소비자 타이틀 화면(150프레임 구간) | 29.5 fps(`wait` 32 ms) → 창·vsync 60.0~60.1, 숨김 60.0, vsync 끔 60.0 fps(`wait` 15 ms) |
| 소비자 데모 도시 장면(300프레임 구간) | 22.4~24.2 fps(CPU 11.6~13.5 ms, `wait` 28 ms) → 39.7~41.6 fps(CPU 10.2~11.6 ms, `wait` 11~13 ms) |
| 소비자 운전 화면(같은 입력 스크립트) | game time 10초에 41.8초(14.4 fps, 실시간의 0.24배) → 20초에 62.9초(19.1 fps, 0.32배). 프레임당 게임 시간은 두 경우 모두 1/60초. 이 기계(i5-7200U, 2코어)에서는 운전 화면의 CPU가 프레임당 33~40 ms라 CPU가 한계다 |

남은 것: 이 기계의 창 모드에서는 호스트 swap이 1 ms 이상 막히지 않아(DWM) 정렬이 한 번도 일어나지 않았다. 정렬 위상의 변경은 설계 §3의 추론으로만 확인했다.

---

# Work log: Swap queues the flip and returns — done (issue #18)

Work order: [20261021-i018-swap-queues-flip](../work-orders/20261021-i018-swap-queues-flip.md) · Design: [20261021-i018-swap-queues-flip](../design/20261021-i018-swap-queues-flip.md) · Consumer: reexec/reCT3#13

Done: (1) The cause, confirmed. The consumer's title screen (0.47 ms of CPU per frame) ran at 29.5 fps with `wait` at 32 ms, and an HLE trace of one frame shows the title's `D3DDevice_BlockUntilVerticalBlank` right after `D3DDevice_Swap`. (2) `d3d8_vblank.{h,cpp}`: `WaitForPresentInterval` became `QueuePresent`, which sleeps only while the previous present's vblank has not come, queues this present's vblank and returns; `kAlignLead` (3 ms before) became `kAlignLag` (1 ms after), so once aligned the virtual vblank follows the host's blank. (3) `d3d8_device.cpp`: both calls in `D3DDevice_Swap` use `QueuePresent`.

Verification: Win32 x86 Release with no errors and CTest passing (5/5).

| Run | Before | After |
|---|---|---|
| Consumer title screen (150-frame spans) | 29.5 fps, `wait` 32 ms | 60.0–60.1 fps in a window with vsync, 60.0 hidden and with vsync off, `wait` 15 ms |
| Consumer demo city scenes (300-frame spans) | 22.4–24.2 fps, CPU 11.6–13.5 ms, `wait` 28 ms | 39.7–41.6 fps, CPU 10.2–11.6 ms, `wait` 11–13 ms |
| Consumer driving, same input script | 10 s of game time in 41.8 s (14.4 fps, 0.24× real time) | 20 s in 62.9 s (19.1 fps, 0.32×) |

Game time per frame is 1/60 s in both driving runs. On this machine (i5-7200U, two cores) driving takes 33–40 ms of CPU per frame, so the CPU is the limit there.

Left over: in a window on this machine the host swap never blocked for 1 ms or more (DWM), so the alignment never ran. The change of its phase is backed only by design section 3's reasoning.
