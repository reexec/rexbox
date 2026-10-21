# 작업 지시: Swap은 플립을 예약하고 돌아온다 (issue #18)

설계: [20261021-i018-swap-queues-flip](../design/20261021-i018-swap-queues-flip.md) · 소비자: reexec/reCT3#13

1. 작업 브랜치 `i018-swap-queues-flip`. 커밋 제목 끝에 `(#18)`.
2. `src/hle/d3d8/d3d8_vblank.{h,cpp}`: `WaitForPresentInterval`을 `QueuePresent`로 바꾸고(설계 §3), `kAlignLead`를 `kAlignLag`로 바꿔 정렬 위상을 호스트 blank 뒤로 옮긴다.
3. `src/hle/d3d8/d3d8_device.cpp`: `D3DDevice_Swap`의 두 호출을 `QueuePresent`로.
4. 검증: Win32 x86 빌드, CTest, 소비자 실행에서 설계 §4의 측정.
5. 작업 로그 `docs/work-logs/20261021-i018-swap-queues-flip.md`.

---

# Work order: Swap queues the flip and returns (issue #18)

Design: [20261021-i018-swap-queues-flip](../design/20261021-i018-swap-queues-flip.md) · Consumer: reexec/reCT3#13

(1) Task branch `i018-swap-queues-flip`; commit titles end with `(#18)`. (2) `src/hle/d3d8/d3d8_vblank.{h,cpp}`: `WaitForPresentInterval` becomes `QueuePresent` (design section 3), and `kAlignLead` becomes `kAlignLag`, moving the alignment phase after the host's blank. (3) `src/hle/d3d8/d3d8_device.cpp`: both calls in `D3DDevice_Swap` use `QueuePresent`. (4) Verify: Win32 x86 build, CTest, and design section 4's measurements in the consumer. (5) Work log `docs/work-logs/20261021-i018-swap-queues-flip.md`.
