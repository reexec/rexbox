# 작업 로그: Linux에서 soft-dirty 쓰기 추적을 쓰지 않는다 (issue #36)

설계: [20261027-i036](../design/20261027-i036-linux-write-tracking.md) · 작업 지시: [20261027-i036](../work-orders/20261027-i036-linux-write-tracking.md) · 소비자: nworkers/reJSRF#125

## 한 일

1. 원인 측정(소비자 reJSRF, 실제 Ubuntu 26.04): 데모 약 4 fps, upload 단계 약 230 ms/프레임. `strace -T`로 `/proc/self/clear_refs` 쓰기가 초당 약 490번, 한 번에 약 1.8 ms임을 확인. LD_PRELOAD로 `clear_refs` 열기를 막아 추적 없는 경로로 돌리면 약 60 fps.
2. `src/platform/linux/virtual_memory_linux.cpp`: soft-dirty 자가 검사, `ClearSoftDirty`, `ReadPagemap`, 추적 블록 목록을 지웠다. `AllocateVirtualMemory`는 `track_writes`를 무시하고 `TakeWrittenPages`는 false를 돌려준다. 공개 계약은 그대로다.
3. 문서: `ARCHITECTURE.md`의 Linux 플랫폼 설명, [설계 i004](../design/20261020-i004-linux-preparation.md)의 가상 메모리 행(대체됨 표시), `docs/guides/linux-setup-and-build.md`.

## 검증

Ubuntu 26.04(실제 머신, Wayland, NVIDIA GeForce RTX 4090, 드라이버 595.99.02, 커널 7.0), GCC 15.2, CMake 4.2.3, Ninja.

| 항목 | 결과 |
|---|---|
| rexbox 단독 Linux x64 Release / Debug | 경고 0, `ctest` 4/4 통과 |
| rexbox 단독 Linux x86 Release | 이 머신에 SDL이 요구하는 i386 개발 패키지가 다 있지 않아 configure 단계에서 멈춤(코드와 무관). CI에서 확인 |
| 소비자 reJSRF x64 Release(submodule을 이 커밋으로) | 경고 0, `ctest` 4/4, `rejsrf_decomp_check` "129 case(s), 0 difference(s)", `rejsrf_d3d8_check` "14018 case(s), 0 difference(s)" |
| 데모 프레임 시간(`REXBOX_D3D8_PROFILE=300`, 숨긴 창, 소리 없음) | 전: 약 4 fps, CPU 약 250 ms, upload 약 230 ms. 후: 59.4~60.0 fps, CPU 2.1~3.2 ms, upload 0.2~0.4 ms, 비교 2.5~4.3 MB/프레임 |
| 그림 | 프레임 3000(데모 위의 JSRF 로고 화면)이 정상으로 그려짐 |

## 남은 것

* 범위별 추적(`PAGEMAP_SCAN` + userfaultfd 비동기 쓰기 보호)은 필요해지면 별도 issue로.

---

# Work log: no soft-dirty write tracking on Linux (issue #36)

Design: [20261027-i036](../design/20261027-i036-linux-write-tracking.md) · Work order: [20261027-i036](../work-orders/20261027-i036-linux-write-tracking.md) · Consumer: nworkers/reJSRF#125

## Done

1. Measured the cause (consumer reJSRF, a real Ubuntu 26.04): the demo at about 4 fps, the upload phase about 230 ms a frame; `strace -T` showed `/proc/self/clear_refs` written about 490 times a second at about 1.8 ms each. With LD_PRELOAD refusing to open `clear_refs`, the untracked path ran at about 60 fps.
2. `src/platform/linux/virtual_memory_linux.cpp`: removed the soft-dirty self test, `ClearSoftDirty`, `ReadPagemap` and the tracked-block list. `AllocateVirtualMemory` ignores `track_writes` and `TakeWrittenPages` returns false. The public contract is unchanged.
3. Documents: the Linux platform description in `ARCHITECTURE.md`, the virtual-memory row of [design i004](../design/20261020-i004-linux-preparation.md) (marked replaced), `docs/guides/linux-setup-and-build.md`.

## Verification

Ubuntu 26.04 (a real machine, Wayland, NVIDIA GeForce RTX 4090, driver 595.99.02, kernel 7.0), GCC 15.2, CMake 4.2.3, Ninja.

| Item | Result |
|---|---|
| rexbox alone, Linux x64 Release / Debug | 0 warnings, `ctest` 4/4 passed |
| rexbox alone, Linux x86 Release | stops at configure: this machine lacks some i386 development packages SDL needs (unrelated to the code); checked by CI |
| Consumer reJSRF x64 Release (submodule on this commit) | 0 warnings, `ctest` 4/4, `rejsrf_decomp_check` "129 case(s), 0 difference(s)", `rejsrf_d3d8_check` "14018 case(s), 0 difference(s)" |
| Demo frame time (`REXBOX_D3D8_PROFILE=300`, hidden window, no audio) | before: about 4 fps, CPU about 250 ms, upload about 230 ms; after: 59.4–60.0 fps, CPU 2.1–3.2 ms, upload 0.2–0.4 ms, 2.5–4.3 MB compared a frame |
| Picture | frame 3000 (the JSRF logo over the demo) draws correctly |

## Left

* Per-range tracking (`PAGEMAP_SCAN` + userfaultfd async write protection) as its own issue if it becomes needed.
