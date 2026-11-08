# 작업 지시: 느린 프레임 보고에 대기·잠금·파일 읽기 (issue #80)

설계: [20261108-i080](../design/20261108-i080-slow-frame-waits.md)

1. 브랜치 `i080-slow-frame-waits`(`origin/main` v0.0.28 위). 커밋 제목 끝에 `(#80)`.
2. `include/rexbox/runtime/thread_activity.h`, `src/runtime/thread_activity.cpp`: `EnableWaitTotals`, 바깥쪽 대기의 합계, `NoteFileRead`, `TakeWaitTotals`(설계 §2.1).
3. `src/hle/kernel/kernel_file.cpp`: `NtReadFile`·`NtReadFileScatter`에서 `NoteFileRead`.
4. `src/hle/d3d8/d3d8_profile.cpp`: 켜기, 프레임마다 받기, 느린 프레임 줄(설계 §2.2). 프로파일 헤더 주석.
5. 빌드, probe, CTest. reCT3 주행 측정(설계 §4).
6. 작업 로그, 릴리스 노트.

---

# Work order: waits, locks and file reads in the slow-frame report (issue #80)

Design: [20261108-i080](../design/20261108-i080-slow-frame-waits.md)

(1) Branch `i080-slow-frame-waits`, on `origin/main` (v0.0.28); commit titles end with `(#80)`. (2) `include/rexbox/runtime/thread_activity.h` and `src/runtime/thread_activity.cpp`: `EnableWaitTotals`, totals of outermost waits, `NoteFileRead`, `TakeWaitTotals` (design section 2.1). (3) `src/hle/kernel/kernel_file.cpp`: `NoteFileRead` in `NtReadFile` and `NtReadFileScatter`. (4) `src/hle/d3d8/d3d8_profile.cpp`: enabling, taking the totals each frame, the slow-frame line (design section 2.2); the profile header's comment. (5) Build, probes, CTest; the reCT3 drive measurement (design section 4). (6) Work log, release notes.
