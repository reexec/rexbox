# 설계: 창 제목과 이벤트 펌프는 창을 만든 스레드에서만 (issue #62)

상태: **구현됨**. 작업 로그: [20261105-i062](../work-logs/20261105-i062-window-thread-title.md). 소비자: reexec/reEXCEED#7.

## 1. 요구사항

`PresentDisplay`는 1초마다 FPS를 창 제목에 쓴다. 이것을 `D3DDevice_Swap`을 부른 게스트 스레드에서, 디스플레이 mutex를 쥔 채 했다. Windows에서 `SDL_SetWindowTitle`은 `SetWindowTextW`이고, 다른 스레드가 만든 창이면 동기 `SendMessage`가 되어 창의 스레드가 메시지를 처리할 때까지 돌아오지 않는다. Pump It Up: Exceed에서는 로딩 화면 스레드(게스트 스레드 3)가 present했고, 창을 만든 게스트 스레드 2는 스레드 3이 쥔 CS를 기다렸다. 그래서 스레드 3이 `SetWindowTextW`에서 영원히 멈췄다(**확인됨**, 호스트 스택). Present마다 부르는 `PumpDisplayEvents`도 다른 스레드에서는 창의 메시지를 받지 못하고, SDL Debug 빌드의 단언("SDL_PumpEvents off main thread")에 걸린다.

## 2. 설계

* `SdlDisplay`에 창을 만든 스레드(`window_thread`)와 대기 중인 제목(`pending_title`)을 둔다.
* 제목을 바꾸는 곳(`PresentDisplay`의 FPS, `SetDisplayTitle`)은 mutex 안에서 대기 제목만 정한다. `ApplyPendingTitle()`이 창의 스레드에서 mutex 없이 `SDL_SetWindowTitle`을 부른다. 다른 스레드의 present가 정한 제목은 창의 스레드가 다음에 present하거나 펌프할 때 반영된다.
* `PumpDisplayEvents`는 창의 스레드가 아니면 아무것도 하지 않는다(`close_requested = false`).
* `CreateDisplay`의 첫 제목은 창을 만드는 스레드가 정하므로 그대로 둔다.

## 3. 검증

* d3d8·kernel probe와 CTest. reEXCEED에서 창을 띄우고 로딩 화면 스레드가 present해도 `SetWindowTextW`에서 멈추지 않는지 확인한다.

---

# Design: the window title and the event pump only on the window's thread (issue #62)

Status: **implemented**. Work log: [20261105-i062](../work-logs/20261105-i062-window-thread-title.md). Consumer: reexec/reEXCEED#7.

**Requirement.** `PresentDisplay` wrote the frame rate into the window title once a second, from whichever guest thread called `D3DDevice_Swap`, with the display mutex held. On Windows `SDL_SetWindowTitle` is `SetWindowTextW`, which for a window created by another thread is a synchronous `SendMessage` that returns only once the window's thread handles it. In Pump It Up: Exceed a loading-screen thread (guest thread 3) presented while the window's thread (guest thread 2) waited on a critical section thread 3 held, so thread 3 stopped in `SetWindowTextW` for good (**confirmed** from host stacks). `PumpDisplayEvents`, called at every present, receives none of the window's messages on another thread either, and trips SDL's Debug assertion ("SDL_PumpEvents off main thread").

**Design.** `SdlDisplay` records the thread that created the window (`window_thread`) and a pending title. The places that change the title (the frame rate in `PresentDisplay`, `SetDisplayTitle`) only set the pending title under the mutex; `ApplyPendingTitle()` calls `SDL_SetWindowTitle` on the window's thread with no mutex held, so a title set by a present on another thread shows at the window thread's next present or pump. `PumpDisplayEvents` does nothing on any other thread (`close_requested = false`). The first title in `CreateDisplay` is set by the thread creating the window and stays as it is.

**Verification.** The d3d8 and kernel probes and CTest; in reEXCEED, with the window shown, a present from the loading-screen thread must not stop in `SetWindowTextW`.
