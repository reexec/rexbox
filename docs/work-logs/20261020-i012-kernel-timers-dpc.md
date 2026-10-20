# 작업 로그: 발화하는 커널 타이머, DPC 실행, `KeWaitForMultipleObjects` (issue #12)

설계: [20261020-i012-kernel-timers-dpc](../design/20261020-i012-kernel-timers-dpc.md) · 작업 지시: [같은 이름](../work-orders/20261020-i012-kernel-timers-dpc.md)

## 한 일

1. `src/hle/kernel/kernel_timer.cpp`(새 파일): 타이머 서비스 호스트 스레드(첫 `KeSetTimer*` 때 시작, detach), `KeSetTimer`/`KeSetTimerEx`/`KeCancelTimer`(`kernel_sync.cpp`에서 옮김), 발화(dispatcher 잠금 아래 `SignalState = 1`, `notify_all`, 주기 재설정, DPC 큐잉), DPC 큐와 `KeInsertQueueDpc`/`KeRemoveQueueDpc`(`kernel_hal.cpp`에서 옮김), DPC 스레드(`StartKernelGuestThread`, 예약 ordinal `0x3FF`의 합성 주소에 `BindKernelImports`가 `rexbox_DpcLoop`를 바인딩, IRQL 2에서 `CallGuest`).
2. `kernel_sync.cpp`: `KeWaitForMultipleObjects`(객체 포인터 배열로 `WaitAndDeliver`).
3. `kernel_thread.cpp`: `StartKernelGuestThread`(인자·TLS 없는 커널 소유 게스트 스레드).
4. 진단: DPC 스레드 시작과 실패, DPC 루틴마다 첫 실행을 `[kernel]` trace로 한 번 남긴다.
5. `rexbox_kernel_probe`: 타이머·대기·DPC 큐 검사 19개(64 → 83개).
6. 문서: 설계(종료 방식을 구현대로 고침), kb `xbox-kernel-structures.md` §8, `ARCHITECTURE.md`(파일 표, export 107개, probe 83개), 가이드의 probe 수.

작업 지시 5(dispatcher 도우미 노출)는 필요 없었다. `kernel_timer.cpp`가 dispatcher 잠금 아래에서 `SignalState`를 직접 쓰고 `State().signalled`를 깨운다.

## 발견과 수정

* probe가 처음에 주기 타이머 검사에서 실패했다. 발화한 주기 타이머의 새 마감이 "가장 이른 마감" 계산에서 빠져, 다른 타이머가 없으면 서비스 스레드가 끝없이 잤다. 다시 설정할 때 그 마감을 넣도록 고쳤다.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Debug 빌드(reCT3 트리) | 오류 0 |
| `rexbox_kernel_probe` / `rexbox_dsound_probe` / `rexbox_d3d8_probe` | 83 / 48 / 163 checks, 0 failures |
| reCT3 CTest | 5/5 통과 |
| reCT3 `rect3.exe roms`(hidden, no audio) | `KeSetTimerEx`를 지나 프레임 94개(30 fps 간격)까지 돈다. 두 번 실행 모두 다음 정지는 DSOUND `IDirectSoundBuffer_SetFormat`(#9). 그 전에 `KeInsertQueueDpc`는 불리지 않아(DPC 스레드 trace 없음), **DPC 루틴 실행은 reCT3에서 아직 검증되지 않았다** |

## 남은 것

* reCT3가 `KeInsertQueueDpc`(루틴 `000E0A69`)에 닿으면 DPC 스레드 trace와 그 뒤 동작을 확인한다.
* IRQL 경쟁(설계 §3 위험)이 측정되면 2단계 dispatch 잠금.

---

# Work log: kernel timers that fire, DPC delivery, `KeWaitForMultipleObjects` (issue #12)

Design: [20261020-i012-kernel-timers-dpc](../design/20261020-i012-kernel-timers-dpc.md) · Work order: [same name](../work-orders/20261020-i012-kernel-timers-dpc.md)

Done: `src/hle/kernel/kernel_timer.cpp` (new) with the timer-service host thread (started at the first `KeSetTimer*`, detached), `KeSetTimer`/`KeSetTimerEx`/`KeCancelTimer` (moved from `kernel_sync.cpp`), firing (`SignalState = 1` under the dispatcher lock, `notify_all`, period re-arming, DPC queuing), the DPC queue with `KeInsertQueueDpc`/`KeRemoveQueueDpc` (moved from `kernel_hal.cpp`) and the DPC thread (`StartKernelGuestThread` at the synthetic address of the reserved ordinal `0x3FF`, which `BindKernelImports` binds to `rexbox_DpcLoop`; `CallGuest` at IRQL 2); `KeWaitForMultipleObjects` in `kernel_sync.cpp` (`WaitAndDeliver` on the object pointers); `StartKernelGuestThread` in `kernel_thread.cpp` (a kernel-owned guest thread with no arguments or TLS); one `[kernel]` trace each for the DPC thread's start or failure and for each DPC routine's first run; 19 timer, wait and DPC-queue checks in `rexbox_kernel_probe` (64 → 83); documents: the design (shutdown as implemented), kb `xbox-kernel-structures.md` section 8, `ARCHITECTURE.md` (file table, 107 exports, 83 probe checks) and the guide's probe counts. Work-order item 5 (exposing dispatcher helpers) was not needed: `kernel_timer.cpp` writes `SignalState` under the dispatcher lock and wakes `State().signalled` itself.

Found and fixed: the probe first failed the periodic-timer check, because a fired periodic timer's new deadline was left out of the earliest-deadline computation and, with no other timer, the service thread slept forever; re-arming now counts it.

Verification: Win32 x86 Debug build (reCT3 tree) with no errors; the three probes report 83 / 48 / 163 checks with 0 failures; reCT3's CTest passes 5/5; reCT3's `rect3.exe roms` (hidden, no audio) passes `KeSetTimerEx` and runs to 94 frames at 30 fps intervals, both runs then stopping at DSOUND `IDirectSoundBuffer_SetFormat` (#9). `KeInsertQueueDpc` is not called before that (no DPC-thread trace), so **running DPC routines is not yet verified on reCT3**.

Left: when reCT3 reaches `KeInsertQueueDpc` (routine `000E0A69`), check the DPC-thread trace and what follows; the phase 2 dispatch lock if the IRQL race (design section 3 risk) is measured.
