# 작업 로그: F10 게스트 스레드 보고 (issue #89)

설계: [20261123-i089-thread-report-key.md](../design/20261123-i089-thread-report-key.md), 작업 지시: [20261123-i089-thread-report-key.md](../work-orders/20261123-i089-thread-report-key.md)

## 경과

1. reEXCEED의 곡 음악 무음 조사에서 시작했다. 음악 스트림을 세 번째로 flush한 뒤 타이틀이 스트림 공급과 효과음 요청을 모두 멈추었고, 진단 로그를 파일로 쓰면 재현되지 않았다.
2. `display.h`에 `ConsumeThreadReportRequest()`를 더하고, SDL 백엔드가 F10에서 플래그를 세운다. stall 모니터가 100 ms 틱마다 보고 `ReportGuestThreads("[threads]", "F10 pressed")`를 부른다. `REXBOX_STALL_REPORT=0`이어도 모니터가 돌고 present 검사만 꺼진다.

## 검증

* reEXCEED에서 rexbox(`origin/main` v0.0.31 + 이 변경)로 Win32 x86 Release 빌드: 경고 0, CTest 7/7(rexbox probe 4개 포함).
* 실행 중 창에 F10 키 메시지(`WM_SYSKEYDOWN`/`WM_SYSKEYUP`, `VK_F10`)를 보내자 `[threads] F10 pressed; guest threads:`와 스레드 4개의 줄(대기 대상, 마지막 HLE, 스택 추정)이 나왔다. 외부 프로세스의 `SendKeys`/`keybd_event`는 포커스를 얻지 못해 F9도 닿지 않았으므로, 창 메시지로 확인했다.
* 이 작업은 사용자 결정으로 Claude가 구현했다(다음 작업부터는 Antigravity가 구현).

---

# Work log: the F10 guest thread report (issue #89)

Design and work order: [design](../design/20261123-i089-thread-report-key.md), [work order](../work-orders/20261123-i089-thread-report-key.md)

## Progress

1. It started from reEXCEED's silent song music: after the music stream's third flush the title stopped feeding the stream and asking for sound effects, and with the diagnostic log written to a file it did not happen.
2. `display.h` gains `ConsumeThreadReportRequest()`, and the SDL backend sets its flag on F10. The stall monitor checks it at each 100 ms tick and calls `ReportGuestThreads("[threads]", "F10 pressed")`. With `REXBOX_STALL_REPORT=0` the monitor still runs, with only the present check off.

## Verification

* reEXCEED Win32 x86 Release with this rexbox (`origin/main` v0.0.31 plus the change): 0 warnings, CTest 7/7 (including rexbox's four probes).
* An F10 key message posted to the running window (`WM_SYSKEYDOWN`/`WM_SYSKEYUP`, `VK_F10`) printed `[threads] F10 pressed; guest threads:` and the four threads' lines (wait objects, last HLE, stack guess). `SendKeys`/`keybd_event` from another process did not get focus (F9 did not arrive either), so window messages were used.
* Claude implemented this task by the user's decision (Antigravity implements from the next task on).
