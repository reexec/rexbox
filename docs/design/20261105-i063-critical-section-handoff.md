# 설계: RtlLeaveCriticalSection이 기다리는 스레드에 소유권을 넘긴다 (issue #63)

상태: **구현됨**. 작업 로그: [20261105-i063](../work-logs/20261105-i063-critical-section-handoff.md). 소비자: reexec/reEXCEED#7.

## 1. 요구사항

`RtlLeaveCriticalSection`은 소유자를 비우고 대기자를 깨우기만 했다. 대기자가 깨어나 `RecursionCount == 0`을 보기 전에 놓은 스레드가 다시 `Enter`하면 그 스레드가 먼저 들어간다. Pump It Up: Exceed의 로딩 화면 스레드는 `Enter` → 그리기 → `Leave` → 0 지연을 반복하므로, 이 CS를 기다리는 메인 스레드가 계속 굶었다(**확인됨**: `[kernel] RtlEnterCriticalSection 008433AC: waiting 5 s; owner <로딩 스레드> ... lock count 1`).

콘솔(NT와 같음)에서는 `LockCount`로 놓는 소유자가 대기자의 존재를 알고, 섹션의 이벤트가 그 대기자를 새 소유자로 깨운다.

## 2. 설계

* 섹션마다 기다리는 스레드의 FIFO(`g_cs_waiters`, `KernelState::dispatcher`로 보호)를 둔다.
* `RtlEnterCriticalSection`: 같은 스레드의 재진입은 전과 같다. 섹션이 비어 있고 대기열도 비어 있으면 바로 소유한다. 아니면 대기열에 들어가 소유자가 자기가 될 때까지 기다린다.
* `RtlLeaveCriticalSection`: 마지막 `Leave`에서 대기열의 첫 스레드를 소유자로 만든다(`OwningThread`, `RecursionCount = 1`). 대기자가 없으면 전처럼 비운다.
* `RtlTryEnterCriticalSection`: 대기열이 있는 섹션은 잡지 않는다.
* `LockCount` 계산은 전과 같다.

## 3. 검증

* kernel probe(한 스레드의 재진입, 해제, try-enter)와 CTest. probe는 게스트 스레드를 돌리지 않으므로 넘겨주기 자체는 reEXCEED에서 확인한다: 로딩 화면 스레드가 반복하는 동안 메인 스레드가 섹션을 얻는다.

---

# Design: RtlLeaveCriticalSection hands the section to a waiting thread (issue #63)

Status: **implemented**. Work log: [20261105-i063](../work-logs/20261105-i063-critical-section-handoff.md). Consumer: reexec/reEXCEED#7.

**Requirement.** `RtlLeaveCriticalSection` cleared the owner and woke the waiters without handing the section to any of them; a thread that left and entered again before a waiter woke up and saw `RecursionCount == 0` got back in first. Pump It Up: Exceed's loading-screen thread loops `Enter` → draw → `Leave` → a zero delay, so the main thread waiting on that section starved (**confirmed**: `[kernel] RtlEnterCriticalSection 008433AC: waiting 5 s; owner <loading thread> ... lock count 1`). On the console (as on NT) `LockCount` tells a leaving owner a thread is waiting, and the section's event wakes that waiter as the new owner.

**Design.** A FIFO of the threads waiting on each section (`g_cs_waiters`, guarded by `KernelState::dispatcher`). `RtlEnterCriticalSection` keeps the recursive path; it takes a free section at once only when no thread is queued, and otherwise queues and waits until it is the owner. The last `RtlLeaveCriticalSection` makes the first queued thread the owner (`OwningThread`, `RecursionCount = 1`), or clears the section as before when none waits. `RtlTryEnterCriticalSection` does not take a section that has a queue. `LockCount` is counted as before.

**Verification.** The kernel probe (one thread's recursion, release and try-enter) and CTest; the probe runs no guest threads, so the hand-off itself is checked in reEXCEED: the main thread gets the section while the loading-screen thread keeps looping.
