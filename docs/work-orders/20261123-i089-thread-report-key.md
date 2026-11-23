# 작업 지시: F10 게스트 스레드 보고 (issue #89)

설계: [20261123-i089-thread-report-key.md](../design/20261123-i089-thread-report-key.md)

1. `include/rexbox/platform/display.h`에 `ConsumeThreadReportRequest()`, `src/platform/sdl/display_sdl.cpp`에 F10 플래그.
2. `src/runtime/thread_activity.cpp`: 모니터 틱마다 요청을 보고 `ReportGuestThreads`. `REXBOX_STALL_REPORT=0`에서도 모니터를 띄우고 present 검사만 끈다. 헤더 주석 갱신.
3. `ARCHITECTURE.md`에 F10을 적는다.
4. Release 빌드, CTest, 소비자 실행 확인. 작업 로그.

---

# Work order: the F10 guest thread report (issue #89)

Design: [20261123-i089-thread-report-key.md](../design/20261123-i089-thread-report-key.md)

1. `ConsumeThreadReportRequest()` in `include/rexbox/platform/display.h`, the F10 flag in `src/platform/sdl/display_sdl.cpp`.
2. `src/runtime/thread_activity.cpp`: check the request at each monitor tick and call `ReportGuestThreads`; start the monitor even with `REXBOX_STALL_REPORT=0`, turning off only the present check; update the header comment.
3. Note F10 in `ARCHITECTURE.md`.
4. Release build, CTest, a consumer run; the work log.
