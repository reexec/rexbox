# 작업 지시: 물리 메모리 창 (issue #8)

설계: [20261020-i008](../design/20261020-i008-physical-window.md)

1. 작업 브랜치 `i008-physical-window`.
2. `include/rexbox/platform/virtual_memory.h`: `AllocateGuestSpace`/`FreeGuestSpace`; Win32(`CreateFileMapping`/`MapViewOfFileEx`)와 Linux(`memfd_create`/`mmap`) 구현.
3. `GuestMemory`: `Options{physical_window}`, `Fold`, `Translate`/`IsValid`, 창을 켠 메모리의 쓰기 추적 끔.
4. `ThreadReport::host_exception_address`.
5. `rexbox_kernel_probe`에 창 검사.
6. 문서: 설계, `docs/kb/xbox-memory-map.md`와 색인, `ARCHITECTURE.md`, 작업 로그.
7. 검증: 설계 §4. reCT3의 `main.cpp`가 옵션을 켠다.

---

# Work order: the physical memory window (issue #8)

Design: [20261020-i008](../design/20261020-i008-physical-window.md)

(1) Task branch `i008-physical-window`. (2) `AllocateGuestSpace`/`FreeGuestSpace` in the platform contract with the Win32 (`CreateFileMapping`/`MapViewOfFileEx`) and Linux (`memfd_create`/`mmap`) implementations. (3) `GuestMemory`: `Options{physical_window}`, `Fold`, `Translate`/`IsValid`, no write tracking with the window. (4) `ThreadReport::host_exception_address`. (5) A window check in `rexbox_kernel_probe`. (6) Documents: the design, `docs/kb/xbox-memory-map.md` and its index, `ARCHITECTURE.md`, the work log. (7) Verify as in design §4; reCT3's `main.cpp` turns the option on.
