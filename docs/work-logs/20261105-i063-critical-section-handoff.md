# 작업 로그: RtlLeaveCriticalSection이 기다리는 스레드에 소유권을 넘긴다 (issue #63)

설계: [20261105-i063](../design/20261105-i063-critical-section-handoff.md) · 소비자: reexec/reEXCEED#7. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* 추적: 직접 호출 프로파일 빌드(`REXBOX_RECOMP_CALL_PROFILE`, 진단용으로 stall 보고에서 출력하는 임시 코드는 커밋하지 않음)에서 로딩 화면 스레드의 루프(`Enter` → 그리기 → Swap → `Leave` → 0 지연)를 확인했다.
* `src/hle/kernel/kernel_rtl.cpp`: `g_cs_waiters`, `HasWaitersLocked`. `Enter`는 대기열에 들어가 소유권을 받을 때까지 기다린다. `Leave`는 첫 대기자에게 넘긴다. `TryEnter`는 대기열이 있으면 실패한다.

## 검증

* Win32 x86 Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7(kernel probe의 CS 검사 포함).
* reEXCEED: #64와 함께, 로딩 화면 스레드가 도는 동안 메인 스레드가 섹션을 얻어 로딩을 마친다(타이틀에서 START → SELECT MODE → 곡 선택 → 난이도 선택, 120초 동안 멈춤 없음).

---

# Work log: RtlLeaveCriticalSection hands the section to a waiting thread (issue #63)

Design: [20261105-i063](../design/20261105-i063-critical-section-handoff.md) · Consumer: reexec/reEXCEED#7. The design's section 2 stands in for a work order.

Done: tracing — a direct-call profile build (`REXBOX_RECOMP_CALL_PROFILE`; the temporary code printing it in the stall report was not committed) showed the loading-screen thread's loop (`Enter` → draw → Swap → `Leave` → a zero delay). In `src/hle/kernel/kernel_rtl.cpp`: `g_cs_waiters` and `HasWaitersLocked`; `Enter` queues and waits until it is handed the section, `Leave` hands it to the first waiter, `TryEnter` fails while a queue exists.

Verification: Win32 x86 Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7 (including the kernel probe's critical-section checks). In reEXCEED, with #64, the main thread gets the section while the loading-screen thread loops and finishes loading (START at the title → SELECT MODE → song select → difficulty select, no stop in 120 s).
