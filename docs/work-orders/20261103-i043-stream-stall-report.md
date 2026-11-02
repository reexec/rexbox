# 작업 지시: 다시 채워지지 않는 스트림 루프 버퍼 보고 (issue #43)

설계: [20261103-i043-stream-stall-report](../design/20261103-i043-stream-stall-report.md)

1. 브랜치 `i042-combiner-factors`를 이어 쓴다. 커밋 제목 끝에 `(#43)`.
2. 플랫폼 계약 `stack_report.h`, Linux·Win32 구현, CMake.
3. runtime: `NoteThreadName`이 등록, `ReportGuestThreads`, 정지 보고가 같은 출력을 씀.
4. DSOUND 믹서: 바퀴 해시, 보고 조건, 분리한 스레드의 보고. `Play`가 상태를 되돌림.
5. reCT3 데모로 오탐과 출력 확인. ARCHITECTURE, 작업 로그, 릴리스 노트 v0.0.19(#42와 함께).

---

# Work order: reporting a streamed loop buffer no longer refilled (issue #43)

Design: [20261103-i043-stream-stall-report](../design/20261103-i043-stream-stall-report.md)

(1) Continue on branch `i042-combiner-factors`; commit titles end with `(#43)`. (2) The platform contract `stack_report.h`, its Linux and Win32 implementations, CMake. (3) Runtime: `NoteThreadName` registers, `ReportGuestThreads`, the stall report sharing its output. (4) DSOUND mixer: lap hashes, the report condition, the report on a detached thread; `Play` resets the state. (5) Check false positives and the output on the reCT3 demo; ARCHITECTURE, work log, release note v0.0.19 (with #42).
