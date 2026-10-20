# 작업 지시: 발화하는 커널 타이머, DPC 실행, `KeWaitForMultipleObjects` (issue #12)

설계: [20261020-i012-kernel-timers-dpc](../design/20261020-i012-kernel-timers-dpc.md)

1. 작업 브랜치 `i008-physical-window`(진행 중인 브랜치에서 이어서). 커밋 제목 끝에 `(#12)`.
2. `src/hle/kernel/kernel_timer.cpp`(새 파일): 타이머 목록과 서비스 스레드, `KeSetTimer`/`KeSetTimerEx`/`KeCancelTimer`(기존 `kernel_sync.cpp`의 것을 옮긴다), 발화(`SignalState`, `notify_all`, 주기, DPC 큐잉).
3. DPC: 큐, `KeInsertQueueDpc`/`KeRemoveQueueDpc`(`kernel_hal.cpp`에서 옮기거나 큐 함수를 부르게), 예약 ordinal의 합성 주소에 바인딩한 `rexbox_DpcLoop`와 DPC 스레드. 실행기가 스레드 시작 주소의 바인딩을 따르는지 먼저 확인한다.
4. `KeWaitForMultipleObjects`(작업 트리의 구현)를 함께 커밋한다.
5. `kernel_internal.h`: 발화에 필요한 dispatcher 도우미(신호 설정과 notify)를 내부 함수로 노출한다.
6. `rexbox_kernel_probe`: 설계 §4의 검사.
7. 문서: 설계, 작업 로그, `ARCHITECTURE.md`의 커널 동기화 설명, `docs/kb`의 커널 타이머·DPC 항목(있으면 갱신, 없으면 추가하고 색인 갱신).
8. 검증: 설계 §4.

---

# Work order: kernel timers that fire, DPC delivery, `KeWaitForMultipleObjects` (issue #12)

Design: [20261020-i012-kernel-timers-dpc](../design/20261020-i012-kernel-timers-dpc.md)

(1) Continue on the task branch `i008-physical-window`; commit titles end with `(#12)`. (2) `src/hle/kernel/kernel_timer.cpp` (new): the timer list and service thread, `KeSetTimer`/`KeSetTimerEx`/`KeCancelTimer` (moved from `kernel_sync.cpp`), firing (`SignalState`, `notify_all`, periods, DPC queuing). (3) DPCs: the queue, `KeInsertQueueDpc`/`KeRemoveQueueDpc` (moved from `kernel_hal.cpp` or calling the queue), `rexbox_DpcLoop` bound at a reserved ordinal's synthetic address, and the DPC thread; first check that the executor follows a binding at a thread's start address. (4) Commit `KeWaitForMultipleObjects` (the working-tree implementation) with it. (5) `kernel_internal.h`: the dispatcher helpers firing needs (setting a signal and notifying) as internal functions. (6) `rexbox_kernel_probe`: the checks of design section 4. (7) Documents: the design, the work log, the kernel synchronization notes in `ARCHITECTURE.md`, a kernel timer and DPC topic in `docs/kb` (updated, or added with its index). (8) Verify as in design section 4.
