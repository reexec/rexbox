# 작업 로그: 느린 프레임 보고에 대기·잠금·파일 읽기 (issue #80)

작업 지시: [20261108-i080](../work-orders/20261108-i080-slow-frame-waits.md) · 설계: [20261108-i080](../design/20261108-i080-slow-frame-waits.md)

## 한 일

1. `6793062`(Antigravity 구현, Claude 설계·검토):
   * `thread_activity`에 스레드별 대기 합계를 더했다(`EnableWaitTotals`, `TakeWaitTotals`). 바깥쪽 대기만 세고, `"D3D8 "` 대기는 뺀다.
   * 파일 읽기 합계(`NoteFileRead`)를 더했다. 커널 `NtReadFile`이 켜져 있을 때만 시간을 잰다.
   * 느린 프레임 줄에 `waits`·`reads`를 붙였다.
2. `NtReadFileScatter`는 이름 표에만 있고 HLE 구현이 없어 재지 않는다(코더 판단, 검토에서 확인).

## 검증

* rexbox CTest 4/4, reCT3 Win32 x86 Release, CTest 7/7. 모두 코더 보고다.
* reCT3 West Coast 주행(`REXBOX_D3D8_SLOW_FRAME=20`)에서 다음을 확인했다.
  * **600프레임마다 50~60 ms 프레임은 대기도 읽기도 없었다.** `REXBOX_D3D8_PROFILE`의 창 보고와 보류된 느린 프레임 보고를 쓰는 비용이다. 측정 도구가 만든 가짜이고, #41에서 "주행 중 100 ms"로 본 프레임 3602·4202도 이것이었다.
  * 100~560 ms 프레임은 파일 읽기와 함께 났다(예: 126번, 6.8 MB, 301 ms). 장면 로딩이다.
  * 주행 중 느린 프레임(가짜 제외 91개)의 가장 큰 항목: draw 단계 45, present 24(602·300·186 ms 포함), 게임 자체 시간 10(대기·읽기 0), 수직 귀선 대기 12.

## 남은 것

* 프로파일 보고 쓰기를 present하는 스레드 밖으로 옮기는 일(측정의 가짜 멈춤 제거).

---

# Work log: waits, locks and file reads in the slow-frame report (issue #80)

Work order: [20261108-i080](../work-orders/20261108-i080-slow-frame-waits.md) · Design: [20261108-i080](../design/20261108-i080-slow-frame-waits.md)

## Done

1. `6793062` (implemented by Antigravity, designed and reviewed by Claude):
   * `thread_activity` gained per-thread wait totals (`EnableWaitTotals`, `TakeWaitTotals`), counting outermost waits only and leaving out the `"D3D8 "` waits;
   * file-read totals (`NoteFileRead`), timed in the kernel's `NtReadFile` only when enabled;
   * slow-frame lines gained `waits` and `reads`.
2. `NtReadFileScatter` is only in the name table, with no HLE implementation, so it is not timed (the coder's call, checked in review).

## Verification

* rexbox CTest 4/4; reCT3 Win32 x86 Release, CTest 7/7. All of these are the coder's report.
* A reCT3 West Coast drive (`REXBOX_D3D8_SLOW_FRAME=20`) showed:
  * **The 50–60 ms frames every 600 frames had no waits and no reads.** They are the cost of writing `REXBOX_D3D8_PROFILE`'s window report and the held slow-frame reports: an artifact of the measurement. Frames 3602 and 4202, seen in #41 as "100 ms during driving", were this too.
  * The 100–560 ms frames came with file reads (e.g. 126 reads, 6.8 MB, 301 ms): scene loading.
  * Driving's slow frames (91 without the artifact), by largest item: draw phases 45, present 24 (including 602, 300 and 186 ms), the title's own time 10 (no waits or reads), the vertical-blank wait 12.

## Remaining

* Moving the profile's report writing off the presenting thread (removing the measurement's artifact stalls).
