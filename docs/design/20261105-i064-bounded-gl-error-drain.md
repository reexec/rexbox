# 설계: GL 오류 비우기에 상한 (issue #64)

상태: **구현됨**. 작업 로그: [20261105-i064](../work-logs/20261105-i064-bounded-gl-error-drain.md). 소비자: reexec/reEXCEED#7. 후속: [#65](https://github.com/reexec/rexbox/issues/65).

## 1. 요구사항

`GetVideoMemoryInfo`는 `while (gl.GetError() != 0) {}`로 GL 오류를 비웠다. GL 컨텍스트는 디스플레이를 만든 스레드에서만 current다. 다른 게스트 스레드가 present하면 overlay의 통계(reEXCEED `SettingsUi::Record`)가 그 스레드에서 이 함수를 부르는데, 거기서는 `glGetError`가 0을 돌려주지 않아 present가 끝나지 않았다(**확인됨**, 호스트 스택 `glGetError <- GetVideoMemoryInfo <- SettingsUi::Record <- RenderOverlay <- PresentRenderTarget <- D3DDevice_Swap`).

## 2. 설계

* `DrainGlErrors(gl)`: 최대 16번 읽는다. 정상 컨텍스트는 오류마다 한 번씩 보고하므로 몇 번 안에 비고, 컨텍스트가 없거나 잃은 경우에만 상한에 닿는다. 비었는지를 돌려준다.
* `GetVideoMemoryInfo`: 처음 비우기가 실패하면 드라이버 조회(NVX·ATI)를 건너뛰고, 드라이버 수치는 모르는 것으로 둔다. 끝의 비우기도 같은 함수를 쓴다.
* 두 번째 스레드의 GL 작업이 효과가 없는 문제는 #65에서 설계한다.

## 3. 검증

* d3d8 probe와 CTest. reEXCEED에서 로딩 화면 스레드가 present해도 멈추지 않는지 확인한다.

---

# Design: a bound on the GL error drain (issue #64)

Status: **implemented**. Work log: [20261105-i064](../work-logs/20261105-i064-bounded-gl-error-drain.md). Consumer: reexec/reEXCEED#7. Follow-up: [#65](https://github.com/reexec/rexbox/issues/65).

**Requirement.** `GetVideoMemoryInfo` emptied the GL error queue with `while (gl.GetError() != 0) {}`. The GL context is current only on the thread that created the display; when another guest thread presents, the overlay's stats (reEXCEED's `SettingsUi::Record`) call this there, `glGetError` never returns 0 and the present never ends (**confirmed**, host stack `glGetError <- GetVideoMemoryInfo <- SettingsUi::Record <- RenderOverlay <- PresentRenderTarget <- D3DDevice_Swap`).

**Design.** `DrainGlErrors(gl)` reads at most 16 times; a working context reports each error once and empties within a few reads, so only a missing or lost context reaches the bound. It returns whether the queue emptied. `GetVideoMemoryInfo` skips the driver queries (NVX, ATI) when the first drain fails, leaving the driver figures unknown, and uses the same function for the final drain. That GL work from a second thread has no effect is designed under #65.

**Verification.** The d3d8 probe and CTest; in reEXCEED a present from the loading-screen thread must not stop.
