# 작업 로그: 창 제목과 이벤트 펌프는 창을 만든 스레드에서만 (issue #62)

설계: [20261105-i062](../design/20261105-i062-window-thread-title.md) · 소비자: reexec/reEXCEED#7. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* 추적: Windows의 stall 보고에는 호스트 스택이 없어서(`PrintThreadStacks` 미구현), 트리 밖의 dbghelp 도구(`StackWalk64`, MAP 파일로 이름 풀기)로 실행 중인 프로세스의 스레드 스택을 찍었다. 로딩 화면 스레드가 `SetWindowTextW` → `NtUserMessageCall`에서 멈춰 있었다.
* `src/platform/sdl/display_sdl.cpp`: `window_thread`, `pending_title`, `ApplyPendingTitle()`, `RequestTitleLocked()`, `CountPresentLocked()`. `PumpDisplayEvents`는 창의 스레드에서만 펌프한다.

## 검증

* Win32 x86 Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED: 이 변경 뒤에는 `SetWindowTextW`에서 멈추지 않았다. 남은 멈춤은 #63과 #64에서 해결했다.

---

# Work log: the window title and the event pump only on the window's thread (issue #62)

Design: [20261105-i062](../design/20261105-i062-window-thread-title.md) · Consumer: reexec/reEXCEED#7. The design's section 2 stands in for a work order.

Done: tracing — the Windows stall report has no host stacks (`PrintThreadStacks` is not implemented there), so a dbghelp tool outside the tree (`StackWalk64`, names from the MAP file) took the live process's thread stacks; the loading-screen thread sat in `SetWindowTextW` → `NtUserMessageCall`. In `src/platform/sdl/display_sdl.cpp`: `window_thread`, `pending_title`, `ApplyPendingTitle()`, `RequestTitleLocked()`, `CountPresentLocked()`; `PumpDisplayEvents` pumps on the window's thread only.

Verification: Win32 x86 Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. In reEXCEED nothing stops in `SetWindowTextW` after this change; the stops left were fixed by #63 and #64.
