# 작업 지시: Linux에서 soft-dirty 쓰기 추적을 쓰지 않는다 (issue #36)

설계: [20261027-i036](../design/20261027-i036-linux-write-tracking.md). 소비자: nworkers/reJSRF#125.

1. `src/platform/linux/virtual_memory_linux.cpp`: soft-dirty 경로(자가 검사, `clear_refs`, `pagemap`, 추적 블록 목록)를 지운다. `AllocateVirtualMemory`는 `track_writes`를 무시하고, `TakeWrittenPages`는 false를 돌려준다. 파일 머리 주석에 이유와 설계 링크.
2. 문서: `ARCHITECTURE.md`의 Linux 플랫폼 설명, [설계 i004](../design/20261020-i004-linux-preparation.md)의 가상 메모리 행(이 설계로 대체됨을 표시), `docs/guides/linux-setup-and-build.md`의 soft-dirty 항목.
3. 검증: Linux x64 Release/Debug 빌드와 `ctest`. reJSRF에서 이 커밋으로 데모 프레임 시간 측정, `rejsrf_decomp_check`, `rejsrf_d3d8_check`.
4. 작업 로그, 머지 때 `VERSION` 0.0.16과 릴리스 노트.

---

# Work order: no soft-dirty write tracking on Linux (issue #36)

Design: [20261027-i036](../design/20261027-i036-linux-write-tracking.md). Consumer: nworkers/reJSRF#125.

1. `src/platform/linux/virtual_memory_linux.cpp`: remove the soft-dirty path (self test, `clear_refs`, `pagemap`, tracked-block list). `AllocateVirtualMemory` ignores `track_writes`; `TakeWrittenPages` returns false. The file's head comment gives the reason and links the design.
2. Documents: the Linux platform description in `ARCHITECTURE.md`, the virtual-memory row of [design i004](../design/20261020-i004-linux-preparation.md) (marked as replaced by this design), the soft-dirty item of `docs/guides/linux-setup-and-build.md`.
3. Verify: Linux x64 Release and Debug builds with `ctest`; in reJSRF on this commit, the demo's frame time, `rejsrf_decomp_check` and `rejsrf_d3d8_check`.
4. Work log; at merge, `VERSION` 0.0.16 and the release note.
